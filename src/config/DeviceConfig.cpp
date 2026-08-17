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
constexpr const char* kCustomColor1Key = "color1";
constexpr const char* kCustomColor2Key = "color2";
constexpr const char* kBrightnessKey = "bright";
constexpr const char* kScreenTimeoutKey = "timeout";
constexpr const char* kClockVisibleKey = "clock";
constexpr const char* kMorseDashKey = "mdot";
constexpr const char* kMorseLetterKey = "mletter";
constexpr const char* kMorseWordKey = "mword";
constexpr const char* kMorseControlKey = "mcontrol";
constexpr const char* kPresetMaskKey = "pmask";
constexpr const char* kOutgoingCounterKey = "outctr";

String presetKey(size_t index) {
    return String("p") + String(index);
}
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
    _settings.customColor1 = _prefs.getUInt(kCustomColor1Key, _settings.customColor1) & 0xFFFFFFU;
    _settings.customColor2 = _prefs.getUInt(kCustomColor2Key, _settings.customColor2) & 0xFFFFFFU;
    _settings.brightnessPercent = _prefs.getUChar(kBrightnessKey, _settings.brightnessPercent);
    if (_settings.brightnessPercent < 10 || _settings.brightnessPercent > 100) {
        _settings.brightnessPercent = 100;
    }
    _settings.screenTimeoutSeconds = _prefs.getUShort(kScreenTimeoutKey, 0);
    if (_settings.screenTimeoutSeconds > 3600) _settings.screenTimeoutSeconds = 0;
    _settings.clockVisible = _prefs.getBool(kClockVisibleKey, true);
    _settings.morseTiming.dashThresholdMs = _prefs.getUShort(kMorseDashKey, 300);
    _settings.morseTiming.letterGapMs = _prefs.getUShort(kMorseLetterKey, 700);
    _settings.morseTiming.wordGapMs = _prefs.getUShort(kMorseWordKey, 1500);
    _settings.morseTiming.controlHoldMs = _prefs.getUShort(kMorseControlKey, 1600);

    const uint8_t presetMask = _prefs.getUChar(kPresetMaskKey, 0x1FU);
    for (size_t i = 0; i < _settings.presets.size(); ++i) {
        const String key = presetKey(i);
        if (_prefs.isKey(key.c_str())) _settings.presets[i] = _prefs.getString(key.c_str(), "");
        if ((presetMask & (1U << i)) == 0) _settings.presets[i] = "";
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
    result.customColor1 = _settings.customColor1;
    result.customColor2 = _settings.customColor2;
    result.brightnessPercent = _settings.brightnessPercent;
    result.screenTimeoutSeconds = _settings.screenTimeoutSeconds;
    result.clockVisible = _settings.clockVisible;
    result.morseTiming = _settings.morseTiming;
    result.presets = _settings.presets;
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
    ok &= _prefs.putUInt(kCustomColor1Key, settings.customColor1) > 0;
    ok &= _prefs.putUInt(kCustomColor2Key, settings.customColor2) > 0;
    ok &= _prefs.putUChar(kBrightnessKey, settings.brightnessPercent) > 0;
    ok &= _prefs.putUShort(kScreenTimeoutKey, settings.screenTimeoutSeconds) > 0;
    ok &= _prefs.putBool(kClockVisibleKey, settings.clockVisible) > 0;
    ok &= _prefs.putUShort(kMorseDashKey, settings.morseTiming.dashThresholdMs) > 0;
    ok &= _prefs.putUShort(kMorseLetterKey, settings.morseTiming.letterGapMs) > 0;
    ok &= _prefs.putUShort(kMorseWordKey, settings.morseTiming.wordGapMs) > 0;
    ok &= _prefs.putUShort(kMorseControlKey, settings.morseTiming.controlHoldMs) > 0;

    uint8_t presetMask = 0;
    for (size_t i = 0; i < settings.presets.size(); ++i) {
        if (settings.presets[i].isEmpty()) continue;
        presetMask |= static_cast<uint8_t>(1U << i);
        const String key = presetKey(i);
        ok &= _prefs.putString(key.c_str(), settings.presets[i]) > 0;
    }
    ok &= _prefs.putUChar(kPresetMaskKey, presetMask) > 0;
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

    for (auto& preset : draft.presets) {
        preset.trim();
        if (preset.length() > core::PresetCatalog::kMaxTextLength) {
            preset.remove(core::PresetCatalog::kMaxTextLength);
        }
    }

    if (draft.mqttPort == 0 || draft.utcOffsetMinutes < -840 || draft.utcOffsetMinutes > 840 ||
        static_cast<uint8_t>(draft.accent) >= static_cast<uint8_t>(core::Accent::Count) ||
        draft.customColor1 > 0xFFFFFFU || draft.customColor2 > 0xFFFFFFU ||
        draft.brightnessPercent < 10 || draft.brightnessPercent > 100 ||
        draft.screenTimeoutSeconds > 3600 ||
        draft.morseTiming.dashThresholdMs < 100 || draft.morseTiming.dashThresholdMs > 1000 ||
        draft.morseTiming.letterGapMs < 250 || draft.morseTiming.letterGapMs > 3000 ||
        draft.morseTiming.wordGapMs <= draft.morseTiming.letterGapMs ||
        draft.morseTiming.wordGapMs > 6000 ||
        draft.morseTiming.controlHoldMs < 800 || draft.morseTiming.controlHoldMs > 5000 ||
        draft.morseTiming.controlHoldMs <= draft.morseTiming.dashThresholdMs) return false;

    Settings candidate = _settings;
    candidate.displayName = draft.displayName;
    candidate.mqttHost = draft.mqttHost;
    candidate.mqttPort = draft.mqttPort;
    candidate.mqttUsername = draft.mqttUsername;
    candidate.mqttPassword = draft.mqttPassword;
    candidate.utcOffsetMinutes = draft.utcOffsetMinutes;
    candidate.accent = draft.accent;
    candidate.customColor1 = draft.customColor1;
    candidate.customColor2 = draft.customColor2;
    candidate.brightnessPercent = draft.brightnessPercent;
    candidate.screenTimeoutSeconds = draft.screenTimeoutSeconds;
    candidate.clockVisible = draft.clockVisible;
    candidate.morseTiming = draft.morseTiming;
    candidate.presets = draft.presets;

    if (roomAction == RoomAction::Create) {
        createRoomCredentials(candidate);
    } else if (roomAction == RoomAction::Join) {
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
