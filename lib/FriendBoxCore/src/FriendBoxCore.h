#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace friendbox::core {

enum class Accent : uint8_t {
    Cyan = 0,
    Blue,
    Green,
    Orange,
    Pink,
    Purple,
    Custom1,
    Custom2,
    Count
};

enum class ButtonAction : uint8_t {
    None = 0,
    Tap,
    Hold,
    LongHold
};

struct SlotMeta {
    bool used{false};
    bool read{false};
    uint32_t sequence{0};
};

const char* accentName(Accent accent);
Accent parseAccent(const std::string& value, Accent fallback = Accent::Cyan);
Accent nextAccent(Accent accent);
uint32_t accentRgb(Accent accent, uint32_t custom1Rgb, uint32_t custom2Rgb);
bool parseRgbHex(const std::string& value, uint32_t& rgb);
std::string rgbHex(uint32_t rgb);

ButtonAction classifyRelease(uint32_t heldMs,
                             uint32_t tapMaxMs = 450,
                             uint32_t holdMaxMs = 1200);

bool validGroupCode(const std::string& code);
bool validGroupPassword(const std::string& password);

// Returns -1, 0 or +1. Only numeric MAJOR.MINOR.PATCH strings are valid.
// Invalid versions compare lower than valid versions; two invalid versions compare equal.
int compareVersions(const std::string& a, const std::string& b);
bool isNewerVersion(const std::string& candidate, const std::string& current);

// Chooses an unused slot first, otherwise the oldest read slot, otherwise the oldest slot.
size_t selectReplacementSlot(const std::vector<SlotMeta>& slots);

}  // namespace friendbox::core
