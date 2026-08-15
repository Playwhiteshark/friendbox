#pragma once

#include <Arduino.h>
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

    bool _ready{false};
    uint32_t _lastUiRefresh{0};
    String _lastWifiLabel;
    bool _lastMqttConnected{false};
    size_t _lastUnread{0};
    String _lastClock;

    bool bootSetupRequested();
    bool ensureConfigured(bool forced);
    void handleInput(core::ButtonAction action);
    void handleIncoming();
    void refreshUiIfNeeded();
    String selectedMessageTime() const;
};

}  // namespace friendbox::app
