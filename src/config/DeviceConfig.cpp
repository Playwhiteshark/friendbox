#include "DeviceConfig.h"

#include <esp_system.h>
#include "util/Hash.h"

namespace friendbox::config {
namespace {
constexpr const char* kNamespace = "fbconfig";
constexpr const char* kRoomAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
}

bool DeviceConfig::begin() {
    if (!_prefs.begin(kNamespace, false)) return false;

    _settings.deviceId = _prefs.getString("device", "");
    if (_settings.deviceId.isEmpty()) {
        _settings.deviceId = makeDeviceId();
        _prefs.putString("device", _settings.deviceId);
    }

    _settings.displayName = _prefs.getString("name", "");
    _settings.groupCode = _prefs.getString("group", "");
    _settings.groupPassword = _prefs.getString("gpass", "");
    _settings.roomToken = _prefs.getString("room", "");
    _settings.mqttHost = _prefs.getString("mhost", "");
    _settings.mqttPort = _prefs.getUShort("mport", 8883);
    _settings.mqttUsername = _prefs.getString("muser", "");
    _settings.mqttPassword = _prefs.getString("mpass", "");
    _settings.utcOffsetMinutes = _prefs.getShort("tz", 0);
    _settings.accent = static_cast<core::Accent>(_prefs.getUChar("accent", 0));
    if (static_cast<uint8_t>(_settings.accent) >= static_cast<uint8_t>(core::Accent::Count)) {
        _settings.accent = core::Accent::Cyan;
    }
    _outCounter = _prefs.getUInt("outctr", 0);

    if (core::validGroupCode(_settings.groupCode.c_str()) &&
        core::validGroupPassword(_settings.groupPassword.c_str())) {
        const String storedToken = _settings.roomToken;
        if (deriveRoomToken() && _settings.roomToken != storedToken) {
            _prefs.putString("room", _settings.roomToken);
        }
    } else {
        _settings.roomToken = "";
    }
    return true;
}

String DeviceConfig::makeDeviceId() {
    const uint64_t chip = ESP.getEfuseMac();
    char id[13];
    snprintf(id, sizeof(id), "%012llx", static_cast<unsigned long long>(chip & 0xFFFFFFFFFFFFULL));
    return String(id);
}

bool DeviceConfig::save() {
    if (_settings.displayName.length() > 24) _settings.displayName.remove(24);
    bool ok = true;
    ok &= _prefs.putString("name", _settings.displayName) > 0 || _settings.displayName.isEmpty();
    ok &= _prefs.putString("group", _settings.groupCode) > 0 || _settings.groupCode.isEmpty();
    ok &= _prefs.putString("gpass", _settings.groupPassword) > 0 || _settings.groupPassword.isEmpty();
    ok &= _prefs.putString("room", _settings.roomToken) > 0 || _settings.roomToken.isEmpty();
    ok &= _prefs.putString("mhost", _settings.mqttHost) > 0 || _settings.mqttHost.isEmpty();
    ok &= _prefs.putUShort("mport", _settings.mqttPort) > 0;
    ok &= _prefs.putString("muser", _settings.mqttUsername) > 0 || _settings.mqttUsername.isEmpty();
    ok &= _prefs.putString("mpass", _settings.mqttPassword) > 0 || _settings.mqttPassword.isEmpty();
    ok &= _prefs.putShort("tz", _settings.utcOffsetMinutes) > 0;
    ok &= _prefs.putUChar("accent", static_cast<uint8_t>(_settings.accent)) > 0;
    return ok;
}

bool DeviceConfig::deriveRoomToken() {
    const String material = "friendbox-v1|" + _settings.groupCode + "|" + _settings.groupPassword;
    const String full = util::sha256Hex(material);
    if (full.length() != 64) return false;
    _settings.roomToken = full.substring(0, 32);
    return true;
}

bool DeviceConfig::createRoom() {
    String code;
    code.reserve(6);
    constexpr size_t alphabetLen = 32;
    for (size_t i = 0; i < 6; ++i) code += kRoomAlphabet[esp_random() % alphabetLen];

    char password[7];
    snprintf(password, sizeof(password), "%06u", static_cast<unsigned>(esp_random() % 1000000U));
    _settings.groupCode = code;
    _settings.groupPassword = password;
    return deriveRoomToken() && save();
}

bool DeviceConfig::joinRoom(String code, String password) {
    code.toUpperCase();
    if (!core::validGroupCode(code.c_str()) || !core::validGroupPassword(password.c_str())) return false;
    _settings.groupCode = code;
    _settings.groupPassword = password;
    return deriveRoomToken() && save();
}

void DeviceConfig::clearRoom() {
    _settings.groupCode = "";
    _settings.groupPassword = "";
    _settings.roomToken = "";
    save();
}

uint32_t DeviceConfig::nextOutgoingCounter() {
    ++_outCounter;
    if (_outCounter == 0) ++_outCounter;
    _prefs.putUInt("outctr", _outCounter);
    return _outCounter;
}

bool DeviceConfig::setAccent(core::Accent accent) {
    _settings.accent = accent;
    return _prefs.putUChar("accent", static_cast<uint8_t>(accent)) > 0;
}

}  // namespace friendbox::config
