#pragma once

namespace friendbox::hardware {
class Board {
public:
    static void begin();
    static void setBacklight(bool on);
};
}
