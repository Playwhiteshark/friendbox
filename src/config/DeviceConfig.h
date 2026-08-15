#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "FriendBoxCore.h"

namespace friendbox::config {

struct Settings {
    String deviceId;
    String displayName;
    String groupCode;
    String groupPassword;
    String roomToken;
    String mqttHost;
    uint16_t mqttPort{8883};
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
    bool complete() const { return deviceId.length() > 0 && displayName.length() > 0 && hasRoom() && hasBroker(); }
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

private:
    Preferences _prefs;
    Settings _settings;
    uint32_t _outCounter{0};

    String makeDeviceId();
    bool deriveRoomToken();
};

}  // namespace friendbox::config
