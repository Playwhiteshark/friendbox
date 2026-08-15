#pragma once

#include <Arduino.h>
#include <atomic>

namespace friendbox::update {

enum class OtaState : uint8_t { Idle, Checking, Downloading, Failed };

class OtaUpdater {
public:
    void begin();
    void update(bool networkReady, bool timeValid, bool appHealthy);
    OtaState state() const { return _state.load(std::memory_order_relaxed); }
    String stateLabel() const;

private:
    std::atomic<OtaState> _state{OtaState::Idle};
    std::atomic_bool _taskRunning{false};
    bool _pendingBootValidation{false};
    uint32_t _bootAt{0};
    uint32_t _lastCheckAt{0};

    static void taskEntry(void* arg);
    void runCheck();
    bool fetchManifest(String& version, String& url, String& sha256, size_t& size);
    bool installFirmware(const String& url, const String& expectedSha256, size_t expectedSize);
    bool repoConfigured() const;
    void validateBootIfHealthy(bool appHealthy);
};

}  // namespace friendbox::update
