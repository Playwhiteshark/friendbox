#include "Board.h"
#include <Arduino.h>
#include "BuildConfig.h"

namespace friendbox::hardware {

void Board::begin() {
    pinMode(build::kPeripheralPowerPin, OUTPUT);
    digitalWrite(build::kPeripheralPowerPin, HIGH);
    pinMode(build::kBacklightPin, OUTPUT);
    digitalWrite(build::kBacklightPin, HIGH);
}

void Board::setBacklight(bool on) {
    digitalWrite(build::kBacklightPin, on ? HIGH : LOW);
}

}  // namespace friendbox::hardware
