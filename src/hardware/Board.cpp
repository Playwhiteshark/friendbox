#include "Board.h"
#include <Arduino.h>
#include "BuildConfig.h"

namespace friendbox::hardware {
namespace {
constexpr uint8_t kBacklightChannel = 7;
constexpr uint32_t kBacklightFrequency = 5000;
constexpr uint8_t kBacklightResolution = 8;
uint8_t backlightDuty = 255;
bool backlightOn = true;

void writeBacklight() {
    ledcWrite(kBacklightChannel, backlightOn ? backlightDuty : 0);
}
}

void Board::begin() {
    pinMode(build::kPeripheralPowerPin, OUTPUT);
    digitalWrite(build::kPeripheralPowerPin, HIGH);
    ledcSetup(kBacklightChannel, kBacklightFrequency, kBacklightResolution);
    ledcAttachPin(build::kBacklightPin, kBacklightChannel);
    writeBacklight();
}

void Board::setBacklight(bool on) {
    backlightOn = on;
    writeBacklight();
}

void Board::setBacklightBrightness(uint8_t percent) {
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    backlightDuty = static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255U) / 100U);
    writeBacklight();
}

}  // namespace friendbox::hardware
