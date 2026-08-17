#include "DeviceConfig.h"

#include <esp_system.h>
#include "ServiceConfig.h"
#include "util/Hash.h"

namespace friendbox::config {
namespace {
constexpr const char* kNamespace = "fbconfig";
constexpr const char* kRoomAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
// Protocol identifier, not branding. Changing it would split existing rooms.
constexpr const char* kRoomTokenPrefix = "friendbox-v1|";

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

    loadSettings();
    if (!ensureDeviceId()) return false;
    if (!seedLocalServiceDefaultsIfNeeded()) return false;
    return normalizeStoredRoom();
}

void DeviceConfig::loadSettings() {
    _settings.deviceId = _prefs.getString(kDeviceKey, "");
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
}

bool DeviceConfig::ensureDeviceId() {
    if (!_settings.deviceId.isEmpty()) return true;
    _settings.deviceId = makeDeviceId();
    return _prefs.putString(kDeviceKey, _settings.deviceId) > 0;
}

bool DeviceConfig::normalizeStoredRoom() {
    if (core::validGroupCode(_settings.groupCode.c_str()) &&
        core::validGroupPassword(_settings.groupPassword.c_str())) {
        const String storedToken = _settings.roomToken;
        if (!deriveRoomToken(_settings)) return false;
        if (_settings.roomToken != storedToken &&
            _prefs.putString(kRoomTokenKey, _settings.roomToken) == 0) return false;
    } else {
        _settings.roomToken = "";
    }
    return true;
}

String DeviceConfig::makeDeviceId() const {
    const uint64_t chip = ESP.getEfuseMac();
    char id[13];
    snprintf(id, sizeof(id), "%012llx", static_cast<unsigned long long>(chip & 0xFFFFFFFFFFFFULL));
    return String(id);
}

bool DeviceConfig::hasMeaningfulStoredServiceSettings(const Settings& settings) const {
    // A saved port by itself is not meaningful; older FriendBox builds always
    // had a default port value, so it must not block first-time provisioning.
    return !settings.mqttHost.isEmpty() ||
           !settings.mqttUsername.isEmpty() ||
           !settings.mqttPassword.isEmpty();
}

bool DeviceConfig::saveServiceSettings(const Settings& settings) {
    bool ok = true;
    ok &= _prefs.putString(kMqttHostKey, settings.mqttHost) > 0 || settings.mqttHost.isEmpty();
    ok &= _prefs.putUShort(kMqttPortKey, settings.mqttPort) > 0;
    ok &= _prefs.putString(kMqttUserKey, settings.mqttUsername) > 0 || settings.mqttUsername.isEmpty();
    ok &= _prefs.putString(kMqttPasswordKey, settings.mqttPassword) > 0 || settings.mqttPassword.isEmpty();

    // Mark service configuration as initialized only when there is actual
    // service data. This still allows a completely blank device to be seeded
    // later by a local provisioning build.
    if (hasMeaningfulStoredServiceSettings(settings)) {
        ok &= _prefs.putBool(kServiceInitializedKey, true) > 0;
    }
    return ok;
}

bool DeviceConfig::seedLocalServiceDefaultsIfNeeded() {
    const bool serviceInitialized = _prefs.getBool(kServiceInitializedKey, false);
    const bool hasStoredSettings = hasMeaningfulStoredServiceSettings(_settings);
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

    if (!saveServiceSettings(_settings)) return false;
    _serviceSeededThisBoot = true;
    return true;
}

SettingsDraft DeviceConfig::draft() const {
    SettingsDraft result;
    result.displayName = _settings.displayName;
    result.groupCode = _settings.groupCode;
    result.groupPassword = _settings.groupPassword;
    result.mqttHost = _settings.mqttHost;
    result.mqttPort = _settings.mqttPort;
    result.mqttUsername = _settings.mqttUsername;
    result.mqttPassword = _settings.mqttPassword;
    result.utcOffsetMinutes = _settings.utcOffsetMinutes;
    result.accent = _settings.accent;
    return result;
}

bool DeviceConfig::saveSettings(const Settings& settings) {
    bool ok = true;
    ok &= _prefs.putString(kNameKey, settings.displayName) > 0 || settings.displayName.isEmpty();
    ok &= _prefs.putString(kGroupKey, settings.groupCode) > 0 || settings.groupCode.isEmpty();
    ok &= _prefs.putString(kGroupPasswordKey, settings.groupPassword) > 0 || settings.groupPassword.isEmpty();
    ok &= _prefs.putString(kRoomTokenKey, settings.roomToken) > 0 || settings.roomToken.isEmpty();
    ok &= saveServiceSettings(settings);
    ok &= _prefs.putShort(kTimezoneKey, settings.utcOffsetMinutes) > 0;
    ok &= _prefs.putUChar(kAccentKey, static_cast<uint8_t>(settings.accent)) > 0;
    return ok;
}

bool DeviceConfig::deriveRoomToken(Settings& settings) const {
    const String material = String(kRoomTokenPrefix) + settings.groupCode + "|" + settings.groupPassword;
    const String full = util::sha256Hex(material);
    if (full.length() != 64) return false;
    settings.roomToken = full.substring(0, 32);
    return true;
}

void DeviceConfig::createRoomCredentials(Settings& settings) const {
    String code;
    code.reserve(6);
    constexpr size_t alphabetLen = sizeof(kRoomAlphabet) - 1;
    for (size_t i = 0; i < 6; ++i) code += kRoomAlphabet[esp_random() % alphabetLen];

    char password[7];
    snprintf(password, sizeof(password), "%06u", static_cast<unsigned>(esp_random() % 1000000U));
    settings.groupCode = code;
    settings.groupPassword = password;
}

bool DeviceConfig::apply(SettingsDraft draft, RoomAction roomAction) {
    draft.displayName.trim();
    if (draft.displayName.length() > 24) draft.displayName.remove(24);
    draft.groupCode.trim();
    draft.groupCode.toUpperCase();
    draft.groupPassword.trim();
    draft.mqttHost.trim();
    draft.mqttUsername.trim();

    if (draft.mqttPort == 0 || draft.utcOffsetMinutes < -840 || draft.utcOffsetMinutes > 840 ||
        static_cast<uint8_t>(draft.accent) >= static_cast<uint8_t>(core::Accent::Count)) return false;

    Settings candidate = _settings;
    candidate.displayName = draft.displayName;
    candidate.mqttHost = draft.mqttHost;
    candidate.mqttPort = draft.mqttPort;
    candidate.mqttUsername = draft.mqttUsername;
    candidate.mqttPassword = draft.mqttPassword;
    candidate.utcOffsetMinutes = draft.utcOffsetMinutes;
    candidate.accent = draft.accent;

    if (roomAction == RoomAction::Create) {
        createRoomCredentials(candidate);
    } else {
        if (!core::validGroupCode(draft.groupCode.c_str()) ||
            !core::validGroupPassword(draft.groupPassword.c_str())) return false;
        candidate.groupCode = draft.groupCode;
        candidate.groupPassword = draft.groupPassword;
    }

    if (!deriveRoomToken(candidate) || !candidate.complete() || !saveSettings(candidate)) return false;
    _settings = candidate;
    return true;
}

uint32_t DeviceConfig::nextOutgoingCounter() {
    ++_outCounter;
    if (_outCounter == 0) ++_outCounter;
    _prefs.putUInt(kOutgoingCounterKey, _outCounter);
    return _outCounter;
}

bool DeviceConfig::setAccent(core::Accent accent) {
    if (static_cast<uint8_t>(accent) >= static_cast<uint8_t>(core::Accent::Count)) return false;
    if (_prefs.putUChar(kAccentKey, static_cast<uint8_t>(accent)) == 0) return false;
    _settings.accent = accent;
    return true;
}

}  // namespace friendbox::config
