#include "InputMapper.h"
#include "BuildConfig.h"

namespace friendbox::input {
core::ButtonAction InputMapper::mapRelease(uint32_t heldMs) const {
    return core::classifyRelease(heldMs, build::kTapMaxMs, build::kHoldMaxMs);
}
}
