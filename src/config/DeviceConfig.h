#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <array>
#include "FriendBoxCore.h"
#include "MorseComposer.h"
#include "PresetCatalog.h"
#include "ProductInfo.h"
#include "ServiceConfig.h"

namespace friendbox::config {

struct Settings {
    String deviceId;
    String displayName;
    String groupCode;
    String groupPassword;
    String roomToken;
    String roomName;
    String roomOwnerId;
    String mqttHost;
    uint16_t mqttPort{service::kDefaultMqttTlsPort};
    String mqttUsername;
    String mqttPassword;
    int16_t utcOffsetMinutes{0};
    core::Accent accent{core::Accent::Cyan};
    uint32_t customColor1{0x00DCE6};
    uint32_t customColor2{0xFF64AF};
    uint8_t brightnessPercent{100};
    uint16_t screenTimeoutSeconds{0};
    bool clockVisible{true};
    core::MorseTiming morseTiming{};
    std::array<String, core::PresetCatalog::kCapacity> presets{
        "HELLO", "POKE", "MISS YOU", "CALL ME", "GOOD NIGHT"
    };

    String setupApName() const {
        return String(product::kSetupApPrefix) + deviceId;
    }

    bool hasRoom() const {
        return core::validGroupCode(groupCode.c_str()) &&
               core::validGroupPassword(groupPassword.c_str()) &&
               roomToken.length() == 32;
    }

    bool ownsRoom() const {
        return !deviceId.isEmpty() && roomOwnerId == deviceId;
    }

    bool hasBroker() const {
        return mqttHost.length() > 0 && mqttPort > 0 &&
               mqttUsername.length() > 0 && mqttPassword.length() > 0;
    }

    bool complete() const {
        return deviceId.length() > 0 && displayName.length() > 0 && hasRoom() && hasBroker();
    }
};

// A copy of only the user-editable fields. Setup UIs modify this draft and
// DeviceConfig validates and commits it in one place; they never mutate live
// settings or derived identifiers directly.
struct SettingsDraft {
    String displayName;
    String groupCode;
    String groupPassword;
    String roomName;
    String mqttHost;
    uint16_t mqttPort{service::kDefaultMqttTlsPort};
    String mqttUsername;
    String mqttPassword;
    int16_t utcOffsetMinutes{0};
    core::Accent accent{core::Accent::Cyan};
    uint32_t customColor1{0x00DCE6};
    uint32_t customColor2{0xFF64AF};
    uint8_t brightnessPercent{100};
    uint16_t screenTimeoutSeconds{0};
    bool clockVisible{true};
    core::MorseTiming morseTiming{};
    std::array<String, core::PresetCatalog::kCapacity> presets;
};

enum class RoomAction : uint8_t { Keep, Create, Join };

class DeviceConfig {
public:
    bool begin();
    const Settings& settings() const { return _settings; }
    SettingsDraft draft() const;
    bool apply(SettingsDraft draft, RoomAction roomAction);
    bool applyRoomMetadata(const String& roomName, const String& ownerId);
    bool canEditRoomName() const { return _settings.roomOwnerId.isEmpty() || _settings.ownsRoom(); }
    bool resetUserStatePreservingService();
    uint32_t nextOutgoingCounter();
    bool setAccent(core::Accent accent);

    // True only on the boot where private local defaults were copied to NVS.
    bool serviceSeededThisBoot() const { return _serviceSeededThisBoot; }

private:
    Preferences _prefs;
    Settings _settings;
    uint32_t _outCounter{0};
    bool _serviceSeededThisBoot{false};

    String makeDeviceId() const;
    void loadSettings();
    bool ensureDeviceId();
    bool normalizeStoredRoom();
    bool deriveRoomToken(Settings& settings) const;
    void createRoomCredentials(Settings& settings) const;
    bool saveSettings(const Settings& settings);
    bool seedLocalServiceDefaultsIfNeeded();
    bool saveServiceSettings(const Settings& settings);
    bool hasMeaningfulStoredServiceSettings(const Settings& settings) const;
};

}  // namespace friendbox::config
