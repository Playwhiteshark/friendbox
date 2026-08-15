#pragma once
#include <Arduino.h>

namespace friendbox::network {
class TimeService {
public:
    void begin();
    void update(bool wifiConnected);
    bool valid() const;
    uint32_t epoch() const;
    String clockText(int16_t utcOffsetMinutes) const;
    String messageTime(uint32_t timestamp, int16_t utcOffsetMinutes) const;

private:
    bool _started{false};
};
}
