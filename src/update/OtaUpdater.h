#pragma once

#include <Arduino.h>
#include <atomic>

namespace friendbox::update {

enum class OtaState : uint8_t { Idle, Checking, Downloading, Failed };

enum class OtaError : uint8_t {
    None,
    CertificateBundle,
    TaskStart,
    ManifestClient,
    ManifestRequest,
    ManifestHttp,
    ManifestOverflow,
    ManifestEmpty,
    ManifestJson,
    ManifestSchema,
    ManifestVersion,
    ManifestUrl,
    ManifestHash,
    ManifestSize,
    NoUpdatePartition,
    ImageTooLarge,
    ShaStart,
    OtaBegin,
    FirmwareClient,
    FirmwareRequest,
    FirmwareHttp,
    FirmwareStream,
    FirmwareSize,
    FirmwareHash,
    OtaFinalize,
    BootPartition,
};

class OtaUpdater {
public:
    void begin();
    void update(bool networkReady, bool timeValid, bool appHealthy);
    OtaState state() const { return _state.load(std::memory_order_relaxed); }
    String stateLabel() const;
    String detailLabel() const;

private:
    std::atomic<OtaState> _state{OtaState::Idle};
    std::atomic<OtaError> _error{OtaError::None};
    std::atomic<int> _httpStatus{0};
    std::atomic<int32_t> _espError{0};
    std::atomic_bool _taskRunning{false};
    bool _certificateBundleReady{false};
    bool _pendingBootValidation{false};
    uint32_t _bootAt{0};
    uint32_t _lastCheckAt{0};

    static void taskEntry(void* arg);
    void runCheck();
    bool fetchManifest(String& version, String& url, String& sha256, size_t& size);
    bool installFirmware(const String& url, const String& expectedSha256, size_t expectedSize);
    bool repoConfigured() const;
    bool embeddedCertificateBundleValid() const;
    void validateBootIfHealthy(bool appHealthy);
    void fail(OtaError error, int httpStatus = 0, int32_t espError = 0);
};

}  // namespace friendbox::update
