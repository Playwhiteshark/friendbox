#pragma once
#include <Arduino.h>

namespace friendbox::network {

enum class WifiStatus { Offline, Connecting, Connected };

class WifiService {
public:
    void begin();
    void update();
    WifiStatus status() const;
    bool connected() const;
    String label() const;

private:
    uint32_t _lastReconnect{0};
};

}  // namespace friendbox::network
