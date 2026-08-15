#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "FriendBoxCore.h"
#include "ServiceConfig.h"

namespace friendbox::config {

struct Settings {
    String deviceId;
    String displayName;
    String groupCode;
    String groupPassword;
    String roomToken;
    String mqttHost;
    uint16_t mqttPort{service::kDefaultMqttTlsPort};
    String mqttUsername;
    String mqttPassword;
    int16_t utcOffsetMinutes{0};
    core::Accent accent{core::Accent::Cyan};

    String setupApName() const {
        return "FriendBox-Setup-" + deviceId;
    }

    bool hasRoom() const {
        return core::validGroupCode(groupCode.c_str()) &&
               core::validGroupPassword(groupPassword.c_str()) &&
               roomToken.length() == 32;
    }

    bool hasBroker() const {
        return mqttHost.length() > 0 && mqttPort > 0 &&
               mqttUsername.length() > 0 && mqttPassword.length() > 0;
    }

    bool complete() const {
        return deviceId.length() > 0 && displayName.length() > 0 && hasRoom() && hasBroker();
    }
};

class DeviceConfig {
public:
    bool begin();
    const Settings& settings() const { return _settings; }
    Settings& mutableSettings() { return _settings; }

    bool save();
    bool createRoom();
    bool joinRoom(String code, String password);
    void clearRoom();
    uint32_t nextOutgoingCounter();
    bool setAccent(core::Accent accent);

    // True only on the boot where private local defaults were copied to NVS.
    bool serviceSeededThisBoot() const { return _serviceSeededThisBoot; }

private:
    Preferences _prefs;
    Settings _settings;
    uint32_t _outCounter{0};
    bool _serviceSeededThisBoot{false};

    String makeDeviceId();
    bool deriveRoomToken();
    bool seedLocalServiceDefaultsIfNeeded();
    bool saveServiceSettings();
    bool hasMeaningfulStoredServiceSettings() const;
};

}  // namespace friendbox::config
