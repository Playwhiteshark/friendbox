#pragma once

#include <Arduino.h>
#include "config/DeviceConfig.h"

namespace friendbox::setup {

struct SetupResult {
    bool saved{false};
    bool createdRoom{false};
};

class SetupPortal {
public:
    SetupResult run(config::DeviceConfig& config, bool forced);
};

}  // namespace friendbox::setup
