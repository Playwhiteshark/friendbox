#pragma once

#include <Arduino.h>
#include "PresetCatalog.h"
#include "config/DeviceConfig.h"
#include "display/Display.h"
#include "hardware/ButtonDriver.h"
#include "input/InputMapper.h"
#include "messaging/MessageStore.h"
#include "messaging/MessagingService.h"
#include "network/WifiService.h"
#include "network/TimeService.h"
#include "setup/SetupPortal.h"
#include "ui/Ui.h"
#include "update/OtaUpdater.h"

namespace friendbox::app {

class App {
public:
    void begin();
    void update();

private:
    config::DeviceConfig _config;
    display::Display _display;
    hardware::ButtonDriver _button;
    input::InputMapper _input;
    messaging::MessageStore _store;
    messaging::MessagingService _messaging;
    network::WifiService _wifi;
    network::TimeService _time;
    setup::SetupPortal _setup;
    ui::Ui _ui;
    update::OtaUpdater _ota;
    core::PresetCatalog _presets;

    bool _ready{false};
    String _lastWifiLabel;
    bool _lastMqttConnected{false};
    size_t _lastUnread{0};
    String _lastClock;

    bool initializePersistentState();
    bool initializeRuntimeServices();
    bool bootSetupRequested();
    bool ensureConfigured(bool forced);
    void updateServices();
    void handleButtonRelease(const hardware::ButtonRelease& release);
    void executeIntent(const ui::Intent& intent);
    void sendPreset(size_t index);
    void cycleAccent();
    void handleIncoming();
    bool routeIncoming(const messaging::Message& message);
    void refreshUiIfNeeded();
    void renderUi();
    void showNotice(const String& title, const String& body, uint32_t durationMs = 2200);
    String selectedMessageTime() const;
};

}  // namespace friendbox::app
