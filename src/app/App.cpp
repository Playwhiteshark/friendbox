#include "App.h"

#include <WiFi.h>
#include "BuildConfig.h"
#include "FriendBoxCore.h"
#include "ProductInfo.h"
#include "hardware/Board.h"
#include "ui/UiRenderer.h"

namespace friendbox::app {

void App::begin() {
    Serial.begin(115200);
    delay(50);
    hardware::Board::begin();
    if (!_display.begin()) {
        Serial.println("Display initialization failed");
        return;
    }

    _display.boot();
    _button.begin(build::kButtonPin);
    _ui.begin();

    if (!initializePersistentState()) return;
    if (!ensureConfigured(bootSetupRequested())) {
        _display.fatal("Setup was not completed. Restart and try again.");
        return;
    }
    if (!initializeRuntimeServices()) return;

    _ready = true;
    _ui.markDirty();
    Serial.printf("%s %s ready; device %s\n", product::kName, FRIEND_BOX_VERSION,
                  _config.settings().deviceId.c_str());
}

bool App::initializePersistentState() {
    if (!_config.begin()) {
        _display.fatal("Configuration storage could not be opened.");
        return false;
    }
    if (_config.serviceSeededThisBoot()) {
        Serial.println("MQTT service settings provisioned into NVS from local defaults.");
    }
    if (!_store.begin()) {
        _display.fatal("Message storage could not be opened.");
        return false;
    }
    loadPresets();
    applyDisplaySettings();
    return true;
}

bool App::initializeRuntimeServices() {
    _wifi.begin();
    if (!_messaging.begin(_config)) {
        _display.fatal("Messaging configuration is invalid. Hold the button during boot to repair setup.");
        return false;
    }
    _ota.begin();
    return true;
}

bool App::bootSetupRequested() {
    if (digitalRead(build::kButtonPin) != LOW) return false;
    const uint32_t started = millis();
    while (digitalRead(build::kButtonPin) == LOW) {
        const uint32_t held = millis() - started;
        _display.boot(held >= 1000 ? "keep holding for setup..." : "button held...");
        if (held >= build::kBootSetupHoldMs) return true;
        delay(20);
    }
    return false;
}

bool App::ensureConfigured(bool forced) {
    const String oldRoom = _config.settings().roomToken;
    if (!forced && _config.settings().complete()) return true;

    while (true) {
        _display.setupMode(_config.settings().setupApName());
        const auto result = _setup.run(_config);
        if (result.saved && _config.settings().complete()) {
            if (!oldRoom.isEmpty() && oldRoom != _config.settings().roomToken) _store.clear();
            loadPresets();
            applyDisplaySettings();
            if (result.createdRoom) {
                showNotice("ROOM CREATED",
                           _config.settings().groupCode + "  " + _config.settings().groupPassword,
                           10000);
            }
            return true;
        }
        if (forced && _config.settings().complete()) return true;
        _display.fatal("Setup incomplete. Check your name, room code/password, and MQTT service fields.");
        delay(1500);
        forced = false;
    }
}

void App::update() {
    if (!_ready) {
        delay(20);
        return;
    }

    updateServices();
    handleIncoming();

    hardware::ButtonRelease release;
    if (_button.poll(release)) handleButtonRelease(release);
    _ui.update(millis(), _button.pressed());

    _ota.update(_wifi.connected(), _time.valid(), true);
    updateBacklight();
    refreshUiIfNeeded();
    renderUi();
    delay(2);
}

void App::updateServices() {
    _button.update();
    _wifi.update();
    _time.update(_wifi.connected());
    _messaging.update(_wifi.connected());
}

void App::handleButtonRelease(const hardware::ButtonRelease& release) {
    const uint32_t now = millis();
    if (!_screenAwake) {
        _screenAwake = true;
        _lastInteractionAt = now;
        hardware::Board::setBacklight(true);
        _ui.markDirty();
        return;
    }
    _lastInteractionAt = now;
    if (_ui.morseEntryActive()) {
        _ui.handleMorseRelease(release.heldMs, now);
        return;
    }
    const ui::NavigationContext context{
        _store.count(),
        _store.firstUnreadIndex(),
        _presets.enabledCount(),
    };
    executeIntent(_ui.handleAction(_input.mapRelease(release.heldMs), context));
}

void App::executeIntent(const ui::Intent& intent) {
    switch (intent.type) {
        case ui::IntentType::None:
            return;
        case ui::IntentType::MarkInboxRead:
            if (_store.markRead(intent.index)) _ui.markDirty();
            return;
        case ui::IntentType::SendPreset:
            sendPreset(intent.index);
            return;
        case ui::IntentType::BeginMessage:
            _ui.startMessageComposer(_config.settings().morseTiming);
            return;
        case ui::IntentType::BeginPresetEdit: {
            const std::string* preset = _presets.at(intent.index);
            _ui.startPresetComposer(intent.index, preset ? *preset : std::string(),
                                    _config.settings().morseTiming);
            return;
        }
        case ui::IntentType::SendComposed:
            sendComposed();
            return;
        case ui::IntentType::SavePreset:
            savePreset(intent.index);
            return;
    }
}

void App::sendPreset(size_t index) {
    const std::string* preset = _presets.enabledAt(index);
    if (!preset) return;
    const String text(preset->c_str());
    if (_messaging.sendText(text, _time.valid() ? _time.epoch() : 0)) {
        showNotice("SENT", text);
    } else {
        showNotice("NOT CONNECTED", "Message was not sent");
    }
}

void App::sendComposed() {
    String text(_ui.composedText().c_str());
    text.trim();
    if (text.isEmpty()) return;
    if (_messaging.sendText(text, _time.valid() ? _time.epoch() : 0)) {
        showNotice("SENT", text);
    } else {
        showNotice("NOT CONNECTED", "Message was not sent");
    }
}

void App::savePreset(size_t index) {
    if (index >= core::PresetCatalog::kCapacity) return;
    config::SettingsDraft draft = _config.draft();
    draft.presets[index] = String(_ui.composedText().c_str());
    if (!_config.apply(draft, config::RoomAction::Keep)) {
        showNotice("SETTINGS ERROR", "Preset was not saved");
        return;
    }
    loadPresets();
    const std::string* saved = _presets.at(index);
    showNotice(saved && saved->empty() ? "PRESET DISABLED" : "PRESET SAVED",
               String(index + 1), 1400);
}

void App::loadPresets() {
    core::PresetCatalog::Items items;
    for (size_t i = 0; i < items.size(); ++i) items[i] = _config.settings().presets[i].c_str();
    _presets.load(items);
}

void App::applyDisplaySettings() {
    hardware::Board::setBacklightBrightness(_config.settings().brightnessPercent);
    hardware::Board::setBacklight(true);
    _screenAwake = true;
    _lastInteractionAt = millis();
    _ui.markDirty();
}

void App::updateBacklight() {
    const uint16_t timeout = _config.settings().screenTimeoutSeconds;
    if (!_screenAwake || timeout == 0) return;
    if (millis() - _lastInteractionAt < static_cast<uint32_t>(timeout) * 1000U) return;
    _screenAwake = false;
    hardware::Board::setBacklight(false);
}

void App::handleIncoming() {
    messaging::Message message;
    while (_messaging.pollIncoming(message)) {
        if (routeIncoming(message)) return;
    }
}

bool App::routeIncoming(const messaging::Message& message) {
    switch (message.type) {
        case messaging::MessageType::Text:
            if (_store.containsId(message.id) || !_store.add(message)) return false;
            if (_screenAwake) showNotice("NEW FROM " + message.sender, message.text);
            return true;
    }
    return false;
}

String App::selectedMessageTime() const {
    if (_ui.screen() != ui::Screen::Inbox || _store.count() == 0) return "";
    const auto* item = _store.at(_ui.inboxIndex() % _store.count());
    if (!item) return "";
    return _time.messageTime(item->message.timestamp, _config.settings().utcOffsetMinutes);
}

void App::refreshUiIfNeeded() {
    const String wifi = _wifi.label();
    const bool mqtt = _messaging.connected();
    const size_t unread = _store.unreadCount();
    const String clock = _time.clockText(_config.settings().utcOffsetMinutes);
    const String ota = _ota.stateLabel();
    const String otaDetail = _ota.detailLabel();
    if (wifi != _lastWifiLabel || mqtt != _lastMqttConnected ||
        unread != _lastUnread || clock != _lastClock || ota != _lastOtaLabel ||
        otaDetail != _lastOtaDetail) {
        _lastWifiLabel = wifi;
        _lastMqttConnected = mqtt;
        _lastUnread = unread;
        _lastClock = clock;
        _lastOtaLabel = ota;
        _lastOtaDetail = otaDetail;
        _ui.markDirty();
    }
}

void App::renderUi() {
    const String clock = _time.clockText(_config.settings().utcOffsetMinutes);
    const String wifi = _wifi.label();
    const String mqtt = _messaging.connected() ? "CONNECTED" : "CONNECTING";
    const String ota = _ota.stateLabel();
    const String otaDetail = _ota.detailLabel();
    const String messageTime = selectedMessageTime();
    const ui::RenderContext context{
        _config.settings(), _store, _presets, clock, wifi, mqtt, ota, otaDetail,
        messageTime,
        _messaging.connected(),
    };
    ui::render(_ui, _display, context);
}

void App::showNotice(const String& title, const String& body, uint32_t durationMs) {
    _ui.notice(title.c_str(), body.c_str(), millis(), durationMs);
}

}  // namespace friendbox::app
