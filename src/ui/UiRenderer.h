#pragma once

#include <Arduino.h>
#include "PresetCatalog.h"
#include "Ui.h"
#include "config/DeviceConfig.h"
#include "display/Display.h"
#include "messaging/MessageStore.h"

namespace friendbox::ui {

struct RenderContext {
    const config::Settings& settings;
    const messaging::MessageStore& messages;
    const core::PresetCatalog& presets;
    const String& clock;
    const String& wifi;
    const String& mqtt;
    const String& ota;
    const String& selectedMessageTime;
    bool mqttConnected{false};
};

void render(Ui& ui, display::Display& display, const RenderContext& context);

}  // namespace friendbox::ui
