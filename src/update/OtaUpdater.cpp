#include "OtaUpdater.h"

#include <ArduinoJson.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>
#include <cstring>
#include "BuildConfig.h"
#include "FriendBoxCore.h"
#include "util/Hash.h"

namespace friendbox::update {
namespace {

struct ManifestContext {
    String body;
    bool overflow{false};
};

esp_err_t manifestEvent(esp_http_client_event_t* event) {
    auto* ctx = static_cast<ManifestContext*>(event->user_data);
    if (!ctx || event->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    if (esp_http_client_get_status_code(event->client) != 200) return ESP_OK;
    if (ctx->body.length() + static_cast<size_t>(event->data_len) > 4096) {
        ctx->overflow = true;
        return ESP_FAIL;
    }
    ctx->body.concat(static_cast<const char*>(event->data), event->data_len);
    return ESP_OK;
}

struct FirmwareContext {
    esp_ota_handle_t handle{0};
    mbedtls_sha256_context sha{};
    size_t bytes{0};
    size_t limit{0};
    bool failed{false};
};

esp_err_t firmwareEvent(esp_http_client_event_t* event) {
    auto* ctx = static_cast<FirmwareContext*>(event->user_data);
    if (!ctx || event->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    if (esp_http_client_get_status_code(event->client) != 200) return ESP_OK;
    if (event->data_len <= 0) return ESP_OK;
    if (ctx->bytes + static_cast<size_t>(event->data_len) > ctx->limit) {
        ctx->failed = true;
        return ESP_FAIL;
    }
    if (esp_ota_write(ctx->handle, event->data, event->data_len) != ESP_OK ||
        mbedtls_sha256_update_ret(&ctx->sha,
                              reinterpret_cast<const unsigned char*>(event->data),
                              event->data_len) != 0) {
        ctx->failed = true;
        return ESP_FAIL;
    }
    ctx->bytes += static_cast<size_t>(event->data_len);
    return ESP_OK;
}

String digestToHex(const unsigned char digest[32]) {
    static const char* hex = "0123456789abcdef";
    char output[65];
    for (size_t i = 0; i < 32; ++i) {
        output[i * 2] = hex[digest[i] >> 4];
        output[i * 2 + 1] = hex[digest[i] & 0x0F];
    }
    output[64] = '\0';
    return String(output);
}

}  // namespace

void OtaUpdater::begin() {
    _bootAt = millis();
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state{};
    _pendingBootValidation = running &&
                             esp_ota_get_state_partition(running, &state) == ESP_OK &&
                             state == ESP_OTA_IMG_PENDING_VERIFY;
}

bool OtaUpdater::repoConfigured() const {
    const String repo(build::kGitHubRepository);
    return repo.indexOf('/') > 0 && !repo.startsWith("CHANGE_ME/");
}

void OtaUpdater::validateBootIfHealthy(bool appHealthy) {
    if (!_pendingBootValidation || !appHealthy || millis() - _bootAt < 5000) return;
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
        _pendingBootValidation = false;
    }
}

void OtaUpdater::update(bool networkReady, bool timeValid, bool appHealthy) {
    validateBootIfHealthy(appHealthy);
    if (!build::kOtaEnabled || !repoConfigured() || !networkReady || !timeValid || _taskRunning.load(std::memory_order_relaxed)) return;

    const uint32_t now = millis();
    const bool first = _lastCheckAt == 0 && now >= build::kOtaFirstCheckDelayMs;
    const bool due = _lastCheckAt != 0 && now - _lastCheckAt >= build::kOtaCheckIntervalMs;
    if (!first && !due) return;

    _lastCheckAt = now;
    _taskRunning.store(true, std::memory_order_relaxed);
    if (xTaskCreate(taskEntry, "friendbox-ota", 12288, this, 1, nullptr) != pdPASS) {
        _taskRunning.store(false, std::memory_order_relaxed);
        _state = OtaState::Failed;
    }
}

void OtaUpdater::taskEntry(void* arg) {
    auto* self = static_cast<OtaUpdater*>(arg);
    self->runCheck();
    self->_taskRunning.store(false, std::memory_order_relaxed);
    vTaskDelete(nullptr);
}

void OtaUpdater::runCheck() {
    _state = OtaState::Checking;
    String version, url, sha;
    size_t size = 0;
    if (!fetchManifest(version, url, sha, size)) {
        _state = OtaState::Failed;
        return;
    }
    if (!core::isNewerVersion(version.c_str(), FRIEND_BOX_VERSION)) {
        _state = OtaState::Idle;
        return;
    }

    _state = OtaState::Downloading;
    if (!installFirmware(url, sha, size)) {
        _state = OtaState::Failed;
        return;
    }

    delay(100);
    esp_restart();
}

bool OtaUpdater::fetchManifest(String& version, String& url, String& sha256, size_t& size) {
    const String manifestUrl = "https://github.com/" + String(build::kGitHubRepository) +
                               "/releases/latest/download/manifest.json";
    ManifestContext context;
    esp_http_client_config_t cfg{};
    cfg.url = manifestUrl.c_str();
    cfg.timeout_ms = 15000;
    cfg.crt_bundle_attach = arduino_esp_crt_bundle_attach;
    cfg.event_handler = manifestEvent;
    cfg.user_data = &context;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;
    esp_http_client_set_header(client, "User-Agent", "FriendBox/1");
    const esp_err_t result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (result != ESP_OK || status != 200 || context.overflow || context.body.isEmpty()) return false;

    JsonDocument doc;
    if (deserializeJson(doc, context.body)) return false;
    if ((doc["schema"] | 0) != 1) return false;
    version = String(doc["version"] | "");
    url = String(doc["url"] | "");
    sha256 = String(doc["sha256"] | "");
    size = doc["size"] | 0U;

    const String expectedPrefix = "https://github.com/" + String(build::kGitHubRepository) + "/releases/download/";
    return core::compareVersions(version.c_str(), "0.0.0") >= 0 &&
           url.startsWith(expectedPrefix) && util::isSha256Hex(sha256) &&
           size >= 65536;
}

bool OtaUpdater::installFirmware(const String& url, const String& expectedSha256, size_t expectedSize) {
    const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
    if (!target || expectedSize == 0 || expectedSize > target->size) return false;

    FirmwareContext context;
    context.limit = expectedSize;
    mbedtls_sha256_init(&context.sha);
    if (mbedtls_sha256_starts_ret(&context.sha, 0) != 0) {
        mbedtls_sha256_free(&context.sha);
        return false;
    }
    if (esp_ota_begin(target, expectedSize, &context.handle) != ESP_OK) {
        mbedtls_sha256_free(&context.sha);
        return false;
    }

    esp_http_client_config_t cfg{};
    cfg.url = url.c_str();
    cfg.timeout_ms = 20000;
    cfg.crt_bundle_attach = arduino_esp_crt_bundle_attach;
    cfg.event_handler = firmwareEvent;
    cfg.user_data = &context;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        esp_ota_abort(context.handle);
        mbedtls_sha256_free(&context.sha);
        return false;
    }
    esp_http_client_set_header(client, "User-Agent", "FriendBox/1");
    const esp_err_t result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    unsigned char digest[32]{};
    const bool shaFinished = mbedtls_sha256_finish_ret(&context.sha, digest) == 0;
    mbedtls_sha256_free(&context.sha);
    const String actualSha = digestToHex(digest);

    if (result != ESP_OK || status != 200 || context.failed ||
        context.bytes != expectedSize || !shaFinished ||
        !actualSha.equalsIgnoreCase(expectedSha256)) {
        esp_ota_abort(context.handle);
        return false;
    }

    if (esp_ota_end(context.handle) != ESP_OK) return false;
    return esp_ota_set_boot_partition(target) == ESP_OK;
}

String OtaUpdater::stateLabel() const {
    switch (_state.load(std::memory_order_relaxed)) {
        case OtaState::Checking: return "CHECKING";
        case OtaState::Downloading: return "UPDATING";
        case OtaState::Failed: return "UPDATE FAILED";
        default: return "IDLE";
    }
}

}  // namespace friendbox::update
