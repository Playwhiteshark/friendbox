#pragma once
#include <stdint.h>
#include "FriendBoxCore.h"

namespace friendbox::input {
class InputMapper {
public:
    core::ButtonAction mapRelease(uint32_t heldMs) const;
};
}
