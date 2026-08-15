#include "DeviceConfig.h"

#include <esp_system.h>
#include "ServiceConfig.h"
#include "util/Hash.h"

namespace friendbox::config {
namespace {
constexpr const char* kNamespace = "fbconfig";
constexpr const char* kRoomAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

// NVS keys are intentionally centralized here and kept below NVS's 15-char
// key-name limit.
constexpr const char* kDeviceKey = "device";
constexpr const char* kNameKey = "name";
constexpr const char* kGroupKey = "group";
constexpr const char* kGroupPasswordKey = "gpass";
constexpr const char* kRoomTokenKey = "room";
constexpr const char* kMqttHostKey = "mhost";
constexpr const char* kMqttPortKey = "mport";
constexpr const char* kMqttUserKey = "muser";
constexpr const char* kMqttPasswordKey = "mpass";
constexpr const char* kServiceInitializedKey = "svcinit";
constexpr const char* kTimezoneKey = "tz";
constexpr const char* kAccentKey = "accent";
constexpr const char* kOutgoingCounterKey = "outctr";
}

bool DeviceConfig::begin() {
    if (!_prefs.begin(kNamespace, false)) return false;

    _settings.deviceId = _prefs.getString(kDeviceKey, "");
    if (_settings.deviceId.isEmpty()) {
        _settings.deviceId = makeDeviceId();
        _prefs.putString(kDeviceKey, _settings.deviceId);
    }

    _settings.displayName = _prefs.getString(kNameKey, "");
    _settings.groupCode = _prefs.getString(kGroupKey, "");
    _settings.groupPassword = _prefs.getString(kGroupPasswordKey, "");
    _settings.roomToken = _prefs.getString(kRoomTokenKey, "");
    _settings.mqttHost = _prefs.getString(kMqttHostKey, "");
    _settings.mqttPort = _prefs.getUShort(kMqttPortKey, service::kDefaultMqttTlsPort);
    _settings.mqttUsername = _prefs.getString(kMqttUserKey, "");
    _settings.mqttPassword = _prefs.getString(kMqttPasswordKey, "");
    _settings.utcOffsetMinutes = _prefs.getShort(kTimezoneKey, 0);
    _settings.accent = static_cast<core::Accent>(_prefs.getUChar(kAccentKey, 0));
    if (static_cast<uint8_t>(_settings.accent) >= static_cast<uint8_t>(core::Accent::Count)) {
        _settings.accent = core::Accent::Cyan;
    }
    _outCounter = _prefs.getUInt(kOutgoingCounterKey, 0);

    // Migration-safe bootstrap behavior:
    // - existing NVS service values always win;
    // - local private defaults seed only a never-configured device;
    // - once seeded/configured, future firmware images cannot overwrite them.
    if (!seedLocalServiceDefaultsIfNeeded()) return false;

    if (core::validGroupCode(_settings.groupCode.c_str()) &&
        core::validGroupPassword(_settings.groupPassword.c_str())) {
        const String storedToken = _settings.roomToken;
        if (deriveRoomToken() && _settings.roomToken != storedToken) {
            _prefs.putString(kRoomTokenKey, _settings.roomToken);
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

bool DeviceConfig::hasMeaningfulStoredServiceSettings() const {
    // A saved port by itself is not meaningful; older FriendBox builds always
    // had a default port value, so it must not block first-time provisioning.
    return !_settings.mqttHost.isEmpty() ||
           !_settings.mqttUsername.isEmpty() ||
           !_settings.mqttPassword.isEmpty();
}

bool DeviceConfig::saveServiceSettings() {
    bool ok = true;
    ok &= _prefs.putString(kMqttHostKey, _settings.mqttHost) > 0 || _settings.mqttHost.isEmpty();
    ok &= _prefs.putUShort(kMqttPortKey, _settings.mqttPort) > 0;
    ok &= _prefs.putString(kMqttUserKey, _settings.mqttUsername) > 0 || _settings.mqttUsername.isEmpty();
    ok &= _prefs.putString(kMqttPasswordKey, _settings.mqttPassword) > 0 || _settings.mqttPassword.isEmpty();

    // Mark service configuration as initialized only when there is actual
    // service data. This still allows a completely blank device to be seeded
    // later by a local provisioning build.
    if (hasMeaningfulStoredServiceSettings()) {
        ok &= _prefs.putBool(kServiceInitializedKey, true) > 0;
    }
    return ok;
}

bool DeviceConfig::seedLocalServiceDefaultsIfNeeded() {
    const bool serviceInitialized = _prefs.getBool(kServiceInitializedKey, false);
    const bool hasStoredSettings = hasMeaningfulStoredServiceSettings();
    const bool defaultsAvailable = service::bootstrapDefaultsAvailable();

    if (serviceInitialized) return true;

    // Preserve any values written by older firmware. This is the migration
    // path for devices that already have broker settings but predate svcinit.
    if (hasStoredSettings) {
        return _prefs.putBool(kServiceInitializedKey, true) > 0;
    }

    if (!service::shouldSeedBootstrap(serviceInitialized, hasStoredSettings, defaultsAvailable)) return true;

    _settings.mqttHost = service::kBootstrapDefaults.host;
    _settings.mqttPort = service::kBootstrapDefaults.port;
    _settings.mqttUsername = service::kBootstrapDefaults.username;
    _settings.mqttPassword = service::kBootstrapDefaults.password;

    if (!saveServiceSettings()) return false;
    _serviceSeededThisBoot = true;
    return true;
}

bool DeviceConfig::save() {
    if (_settings.displayName.length() > 24) _settings.displayName.remove(24);
    bool ok = true;
    ok &= _prefs.putString(kNameKey, _settings.displayName) > 0 || _settings.displayName.isEmpty();
    ok &= _prefs.putString(kGroupKey, _settings.groupCode) > 0 || _settings.groupCode.isEmpty();
    ok &= _prefs.putString(kGroupPasswordKey, _settings.groupPassword) > 0 || _settings.groupPassword.isEmpty();
    ok &= _prefs.putString(kRoomTokenKey, _settings.roomToken) > 0 || _settings.roomToken.isEmpty();
    ok &= saveServiceSettings();
    ok &= _prefs.putShort(kTimezoneKey, _settings.utcOffsetMinutes) > 0;
    ok &= _prefs.putUChar(kAccentKey, static_cast<uint8_t>(_settings.accent)) > 0;
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
    _prefs.putUInt(kOutgoingCounterKey, _outCounter);
    return _outCounter;
}

bool DeviceConfig::setAccent(core::Accent accent) {
    _settings.accent = accent;
    return _prefs.putUChar(kAccentKey, static_cast<uint8_t>(accent)) > 0;
}

}  // namespace friendbox::config
