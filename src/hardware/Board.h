#pragma once

#include <cstdint>

namespace friendbox::hardware {
class Board {
public:
    static void begin();
    static void setBacklight(bool on);
    static void setBacklightBrightness(uint8_t percent);
};
}
