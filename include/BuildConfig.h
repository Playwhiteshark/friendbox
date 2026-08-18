#pragma once
#include <stdint.h>
#include "GeneratedVersion.h"

namespace friendbox::build {

// External momentary button wired between this pin and GND.
// Change to 14 to use the T-Display-S3 onboard KEY1 button.
constexpr int kButtonPin = 1;
constexpr int kPeripheralPowerPin = 15;
constexpr int kBacklightPin = 38;

constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kTapMaxMs = 450;
constexpr uint32_t kHoldMaxMs = 1200;
constexpr uint32_t kBootSetupHoldMs = 5000;
constexpr uint32_t kBootFactoryResetHoldMs = 10000;

constexpr size_t kMaxStoredMessages = 50;
constexpr size_t kMaxTextBytes = 256;
constexpr size_t kMaxMqttPayloadBytes = 768;
constexpr size_t kMqttRxQueueDepth = 16;

constexpr uint32_t kMqttReconnectMs = 5000;
constexpr uint32_t kWifiReconnectMs = 15000;
constexpr uint32_t kOtaFirstCheckDelayMs = 30000;
constexpr uint32_t kOtaCheckIntervalMs = 12UL * 60UL * 60UL * 1000UL;

// Public repository slug only; never put credentials here.
// Example: "yourname/friendbox"
constexpr const char* kGitHubRepository = "Playwhiteshark/friendbox";
constexpr bool kOtaEnabled = true;

constexpr const char* kDefaultNtp1 = "pool.ntp.org";
constexpr const char* kDefaultNtp2 = "time.nist.gov";

}  // namespace friendbox::build
