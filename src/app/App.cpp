#include "App.h"

#include <WiFi.h>
#include "BuildConfig.h"
#include "FriendBoxCore.h"
#include "hardware/Board.h"

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

    if (!_config.begin()) {
        _display.fatal("Configuration storage could not be opened.");
        return;
    }
    if (_config.serviceSeededThisBoot()) {
        Serial.println("MQTT service settings provisioned into NVS from local defaults.");
    }
    if (!_store.begin()) {
        _display.fatal("Message storage could not be opened.");
        return;
    }

    const bool forcedSetup = bootSetupRequested();
    if (!ensureConfigured(forcedSetup)) {
        _display.fatal("Setup was not completed. Restart and try again.");
        return;
    }

    _wifi.begin();
    _time.begin();
    _ui.begin();
    if (!_messaging.begin(_config, _store)) {
        _display.fatal("Messaging configuration is invalid. Hold the button during boot to repair setup.");
        return;
    }
    _ota.begin();
    _ready = true;
    _ui.openIdle();
    Serial.printf("FriendBox %s ready; device %s\n", FRIEND_BOX_VERSION, _config.settings().deviceId.c_str());
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
        const auto result = _setup.run(_config, forced);
        if (result.saved && _config.settings().complete()) {
            if (!oldRoom.isEmpty() && oldRoom != _config.settings().roomToken) _store.clear();
            if (result.createdRoom) {
                _ui.notice("ROOM CREATED",
                           _config.settings().groupCode + "  " + _config.settings().groupPassword,
                           10000);
            }
            return true;
        }
        if (forced && _config.settings().complete()) return true;  // portal failed: keep known-good maintenance config
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

    _button.update();
    _wifi.update();
    _time.update(_wifi.connected());
    _messaging.update(_wifi.connected());
    _ui.update();
    handleIncoming();

    hardware::ButtonRelease release;
    if (_button.poll(release)) handleInput(_input.mapRelease(release.heldMs));

    _ota.update(_wifi.connected(), _time.valid(), true);
    refreshUiIfNeeded();
    _ui.render(_display, _config.settings(), _store,
               _time.clockText(_config.settings().utcOffsetMinutes),
               _wifi.label(), _messaging.connected() ? "CONNECTED" : "CONNECTING",
               _messaging.connected(), selectedMessageTime());
    delay(2);
}

void App::handleIncoming() {
    messaging::Message message;
    if (_messaging.pollIncoming(message)) {
        _ui.notice("NEW FROM " + message.sender, message.text);
        _ui.markDirty();
    }
}

void App::handleInput(core::ButtonAction action) {
    if (action == core::ButtonAction::None) return;
    switch (_ui.screen()) {
        case ui::Screen::Idle:
            if (action == core::ButtonAction::Tap) _ui.openInbox(_store);
            else if (action == core::ButtonAction::Hold) _ui.openSend();
            else if (action == core::ButtonAction::LongHold) _ui.openInfo();
            break;

        case ui::Screen::Inbox:
            if (action == core::ButtonAction::Tap) _ui.nextInbox(_store);
            else if (action == core::ButtonAction::Hold) {
                _store.markRead(_ui.inboxIndex());
                _ui.markDirty();
            } else if (action == core::ButtonAction::LongHold) _ui.openIdle();
            break;

        case ui::Screen::Send:
            if (action == core::ButtonAction::Tap) _ui.nextSend();
            else if (action == core::ButtonAction::Hold) {
                if (_messaging.sendText(_ui.selectedPreset(), _time.valid() ? _time.epoch() : 0))
                    _ui.notice("SENT", _ui.selectedPreset());
                else
                    _ui.notice("NOT CONNECTED", "Message was not sent");
            } else if (action == core::ButtonAction::LongHold) _ui.openIdle();
            break;

        case ui::Screen::Info:
            if (action == core::ButtonAction::Tap) _ui.nextInfo();
            else if (action == core::ButtonAction::Hold) {
                const auto accent = core::nextAccent(_config.settings().accent);
                _config.setAccent(accent);
                _ui.notice("ACCENT", core::accentName(accent), 1000);
            } else if (action == core::ButtonAction::LongHold) _ui.openIdle();
            break;
    }
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
    if (wifi != _lastWifiLabel || mqtt != _lastMqttConnected || unread != _lastUnread || clock != _lastClock) {
        _lastWifiLabel = wifi;
        _lastMqttConnected = mqtt;
        _lastUnread = unread;
        _lastClock = clock;
        _ui.markDirty();
    }
}

}  // namespace friendbox::app
