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
#include "ProductInfo.h"
#include "util/Hash.h"

namespace friendbox::update {
namespace {

constexpr size_t kMaxManifestBytes = 4096;
constexpr uint32_t kHealthyBootDelayMs = 5000;
constexpr uint32_t kManifestTimeoutMs = 30000;
constexpr uint32_t kFirmwareTimeoutMs = 60000;
constexpr uint32_t kOtaTaskStackBytes = 12288;
constexpr size_t kMinimumFirmwareBytes = 65536;
constexpr int kMaxRedirects = 5;

struct ManifestContext {
    String body;
    bool overflow{false};
};

esp_err_t manifestEvent(esp_http_client_event_t* event) {
    auto* ctx = static_cast<ManifestContext*>(event->user_data);
    if (!ctx || event->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    if (esp_http_client_get_status_code(event->client) != 200) return ESP_OK;
    if (ctx->body.length() + static_cast<size_t>(event->data_len) > kMaxManifestBytes) {
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

const char* errorName(OtaError error) {
    switch (error) {
        case OtaError::TaskStart: return "TASK START";
        case OtaError::ManifestClient: return "MANIFEST CLIENT";
        case OtaError::ManifestRequest: return "MANIFEST REQUEST";
        case OtaError::ManifestHttp: return "MANIFEST HTTP";
        case OtaError::ManifestOverflow: return "MANIFEST TOO LARGE";
        case OtaError::ManifestEmpty: return "MANIFEST EMPTY";
        case OtaError::ManifestJson: return "MANIFEST JSON";
        case OtaError::ManifestSchema: return "MANIFEST SCHEMA";
        case OtaError::ManifestVersion: return "MANIFEST VERSION";
        case OtaError::ManifestUrl: return "MANIFEST URL";
        case OtaError::ManifestHash: return "MANIFEST HASH";
        case OtaError::ManifestSize: return "MANIFEST SIZE";
        case OtaError::NoUpdatePartition: return "NO UPDATE SLOT";
        case OtaError::ImageTooLarge: return "IMAGE TOO LARGE";
        case OtaError::ShaStart: return "SHA START";
        case OtaError::OtaBegin: return "OTA BEGIN";
        case OtaError::FirmwareClient: return "FIRMWARE CLIENT";
        case OtaError::FirmwareRequest: return "FIRMWARE REQUEST";
        case OtaError::FirmwareHttp: return "FIRMWARE HTTP";
        case OtaError::FirmwareStream: return "FIRMWARE STREAM";
        case OtaError::FirmwareSize: return "FIRMWARE SIZE";
        case OtaError::FirmwareHash: return "FIRMWARE HASH";
        case OtaError::OtaFinalize: return "OTA FINALIZE";
        case OtaError::BootPartition: return "BOOT PARTITION";
        default: return "";
    }
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
    if (!_pendingBootValidation || !appHealthy || millis() - _bootAt < kHealthyBootDelayMs) return;
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
    if (xTaskCreate(taskEntry, "friendbox-ota", kOtaTaskStackBytes, this, 1, nullptr) != pdPASS) {
        _taskRunning.store(false, std::memory_order_relaxed);
        fail(OtaError::TaskStart);
    }
}

void OtaUpdater::taskEntry(void* arg) {
    auto* self = static_cast<OtaUpdater*>(arg);
    self->runCheck();
    self->_taskRunning.store(false, std::memory_order_relaxed);
    vTaskDelete(nullptr);
}

void OtaUpdater::runCheck() {
    _error = OtaError::None;
    _httpStatus = 0;
    _espError = 0;
    _state = OtaState::Checking;
    Serial.printf("[OTA] Checking for an update; version=%s free_heap=%u\n",
                  FRIEND_BOX_VERSION, static_cast<unsigned>(ESP.getFreeHeap()));
    String version, url, sha;
    size_t size = 0;
    if (!fetchManifest(version, url, sha, size)) return;
    if (!core::isNewerVersion(version.c_str(), FRIEND_BOX_VERSION)) {
        Serial.printf("[OTA] Current version is up to date; latest=%s\n", version.c_str());
        _state = OtaState::Idle;
        return;
    }

    _state = OtaState::Downloading;
    Serial.printf("[OTA] Downloading version=%s size=%u free_heap=%u\n", version.c_str(),
                  static_cast<unsigned>(size), static_cast<unsigned>(ESP.getFreeHeap()));
    if (!installFirmware(url, sha, size)) return;

    Serial.println("[OTA] Install complete; restarting");
    Serial.flush();
    delay(100);
    esp_restart();
}

void OtaUpdater::fail(OtaError error, int httpStatus, int32_t espError) {
    _error = error;
    _httpStatus = httpStatus;
    _espError = espError;
    _state = OtaState::Failed;
    Serial.printf("[OTA] FAILED stage=%s http=%d esp=%ld free_heap=%u\n",
                  errorName(error), httpStatus, static_cast<long>(espError),
                  static_cast<unsigned>(ESP.getFreeHeap()));
}

bool OtaUpdater::fetchManifest(String& version, String& url, String& sha256, size_t& size) {
    const String manifestUrl = "https://github.com/" + String(build::kGitHubRepository) +
                               "/releases/latest/download/manifest.json";
    ManifestContext context;
    esp_http_client_config_t cfg{};
    cfg.url = manifestUrl.c_str();
    cfg.timeout_ms = kManifestTimeoutMs;
    cfg.max_redirection_count = kMaxRedirects;
    cfg.crt_bundle_attach = arduino_esp_crt_bundle_attach;
    cfg.event_handler = manifestEvent;
    cfg.user_data = &context;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        fail(OtaError::ManifestClient);
        return false;
    }
    esp_http_client_set_header(client, "User-Agent", product::kHttpUserAgent);
    esp_http_client_set_header(client, "Accept", "application/octet-stream");
    const esp_err_t result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (result != ESP_OK) {
        fail(OtaError::ManifestRequest, status, result);
        return false;
    }
    if (status != 200) {
        fail(OtaError::ManifestHttp, status);
        return false;
    }
    if (context.overflow) {
        fail(OtaError::ManifestOverflow, status);
        return false;
    }
    if (context.body.isEmpty()) {
        fail(OtaError::ManifestEmpty, status);
        return false;
    }

    JsonDocument doc;
    if (deserializeJson(doc, context.body)) {
        fail(OtaError::ManifestJson, status);
        return false;
    }
    if ((doc["schema"] | 0) != 1) {
        fail(OtaError::ManifestSchema, status);
        return false;
    }
    version = String(doc["version"] | "");
    url = String(doc["url"] | "");
    sha256 = String(doc["sha256"] | "");
    size = doc["size"] | 0U;

    const String expectedPrefix = "https://github.com/" + String(build::kGitHubRepository) + "/releases/download/";
    if (core::compareVersions(version.c_str(), "0.0.0") < 0) {
        fail(OtaError::ManifestVersion, status);
        return false;
    }
    if (!url.startsWith(expectedPrefix)) {
        fail(OtaError::ManifestUrl, status);
        return false;
    }
    if (!util::isSha256Hex(sha256)) {
        fail(OtaError::ManifestHash, status);
        return false;
    }
    if (size < kMinimumFirmwareBytes) {
        fail(OtaError::ManifestSize, status);
        return false;
    }
    return true;
}

bool OtaUpdater::installFirmware(const String& url, const String& expectedSha256, size_t expectedSize) {
    const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
    if (!target) {
        fail(OtaError::NoUpdatePartition);
        return false;
    }
    if (expectedSize == 0 || expectedSize > target->size) {
        fail(OtaError::ImageTooLarge);
        return false;
    }

    FirmwareContext context;
    context.limit = expectedSize;
    mbedtls_sha256_init(&context.sha);
    if (mbedtls_sha256_starts_ret(&context.sha, 0) != 0) {
        mbedtls_sha256_free(&context.sha);
        fail(OtaError::ShaStart);
        return false;
    }
    const esp_err_t beginResult = esp_ota_begin(target, expectedSize, &context.handle);
    if (beginResult != ESP_OK) {
        mbedtls_sha256_free(&context.sha);
        fail(OtaError::OtaBegin, 0, beginResult);
        return false;
    }

    esp_http_client_config_t cfg{};
    cfg.url = url.c_str();
    cfg.timeout_ms = kFirmwareTimeoutMs;
    cfg.max_redirection_count = kMaxRedirects;
    cfg.crt_bundle_attach = arduino_esp_crt_bundle_attach;
    cfg.event_handler = firmwareEvent;
    cfg.user_data = &context;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        esp_ota_abort(context.handle);
        mbedtls_sha256_free(&context.sha);
        fail(OtaError::FirmwareClient);
        return false;
    }
    esp_http_client_set_header(client, "User-Agent", product::kHttpUserAgent);
    esp_http_client_set_header(client, "Accept", "application/octet-stream");
    const esp_err_t result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    unsigned char digest[32]{};
    const bool shaFinished = mbedtls_sha256_finish_ret(&context.sha, digest) == 0;
    mbedtls_sha256_free(&context.sha);
    const String actualSha = digestToHex(digest);

    if (result != ESP_OK) {
        esp_ota_abort(context.handle);
        fail(OtaError::FirmwareRequest, status, result);
        return false;
    }
    if (status != 200) {
        esp_ota_abort(context.handle);
        fail(OtaError::FirmwareHttp, status);
        return false;
    }
    if (context.failed) {
        esp_ota_abort(context.handle);
        fail(OtaError::FirmwareStream, status);
        return false;
    }
    if (context.bytes != expectedSize) {
        esp_ota_abort(context.handle);
        fail(OtaError::FirmwareSize, status);
        return false;
    }
    if (!shaFinished || !actualSha.equalsIgnoreCase(expectedSha256)) {
        esp_ota_abort(context.handle);
        fail(OtaError::FirmwareHash, status);
        return false;
    }

    const esp_err_t endResult = esp_ota_end(context.handle);
    if (endResult != ESP_OK) {
        fail(OtaError::OtaFinalize, status, endResult);
        return false;
    }
    const esp_err_t bootResult = esp_ota_set_boot_partition(target);
    if (bootResult != ESP_OK) {
        fail(OtaError::BootPartition, status, bootResult);
        return false;
    }
    return true;
}

String OtaUpdater::stateLabel() const {
    switch (_state.load(std::memory_order_relaxed)) {
        case OtaState::Checking: return "CHECKING";
        case OtaState::Downloading: return "UPDATING";
        case OtaState::Failed: return "UPDATE FAILED";
        default: return "IDLE";
    }
}

String OtaUpdater::detailLabel() const {
    const OtaError error = _error.load(std::memory_order_relaxed);
    if (error == OtaError::None) return "";
    String detail(errorName(error));
    const int httpStatus = _httpStatus.load(std::memory_order_relaxed);
    const int32_t espError = _espError.load(std::memory_order_relaxed);
    if (httpStatus != 0) {
        detail += " HTTP ";
        detail += String(httpStatus);
    }
    if (espError != 0) {
        detail += " ERR ";
        detail += String(static_cast<long>(espError));
    }
    return detail;
}

}  // namespace friendbox::update
