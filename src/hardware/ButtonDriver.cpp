#include "ButtonDriver.h"
#include "BuildConfig.h"

namespace friendbox::hardware {

void ButtonDriver::begin(int pin) {
    _pin = pin;
    pinMode(_pin, INPUT_PULLUP);
    _rawPressed = digitalRead(_pin) == LOW;
    _stablePressed = _rawPressed;
    _rawChangedAt = millis();
    if (_stablePressed) _pressedAt = millis();
}

void ButtonDriver::update() {
    if (_pin < 0) return;
    const uint32_t now = millis();
    const bool raw = digitalRead(_pin) == LOW;
    if (raw != _rawPressed) {
        _rawPressed = raw;
        _rawChangedAt = now;
    }

    if (_rawPressed != _stablePressed && now - _rawChangedAt >= build::kDebounceMs) {
        _stablePressed = _rawPressed;
        if (_stablePressed) {
            _pressedAt = now;
        } else {
            _event.heldMs = now - _pressedAt;
            _eventPending = true;
        }
    }
}

bool ButtonDriver::poll(ButtonRelease& event) {
    if (!_eventPending) return false;
    event = _event;
    _eventPending = false;
    return true;
}

}  // namespace friendbox::hardware
