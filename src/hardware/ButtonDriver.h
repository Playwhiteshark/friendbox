#pragma once
#include <Arduino.h>

namespace friendbox::hardware {

struct ButtonRelease {
    uint32_t heldMs{0};
};

class ButtonDriver {
public:
    void begin(int pin);
    void update();
    bool poll(ButtonRelease& event);
    bool pressed() const { return _stablePressed; }
    int pin() const { return _pin; }

private:
    int _pin{-1};
    bool _rawPressed{false};
    bool _stablePressed{false};
    uint32_t _rawChangedAt{0};
    uint32_t _pressedAt{0};
    bool _eventPending{false};
    ButtonRelease _event{};
};

}  // namespace friendbox::hardware
