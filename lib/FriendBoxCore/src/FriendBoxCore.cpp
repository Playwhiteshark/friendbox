#include "FriendBoxCore.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <limits>

namespace friendbox::core {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool parseVersion(const std::string& value, std::array<uint32_t, 3>& out) {
    size_t cursor = 0;
    for (size_t partIndex = 0; partIndex < out.size(); ++partIndex) {
        if (cursor >= value.size()) return false;
        uint32_t parsed = 0;
        size_t digits = 0;
        while (cursor < value.size() && value[cursor] != '.') {
            const unsigned char c = static_cast<unsigned char>(value[cursor]);
            if (!std::isdigit(c)) return false;
            const uint32_t digit = static_cast<uint32_t>(c - '0');
            if (parsed > (std::numeric_limits<uint32_t>::max() - digit) / 10U) return false;
            parsed = parsed * 10U + digit;
            ++cursor;
            ++digits;
        }
        if (digits == 0) return false;
        out[partIndex] = parsed;

        if (partIndex + 1 < out.size()) {
            if (cursor >= value.size() || value[cursor] != '.') return false;
            ++cursor;
        }
    }
    return cursor == value.size();
}

}  // namespace

const char* accentName(Accent accent) {
    switch (accent) {
        case Accent::Cyan: return "cyan";
        case Accent::Blue: return "blue";
        case Accent::Green: return "green";
        case Accent::Orange: return "orange";
        case Accent::Pink: return "pink";
        case Accent::Purple: return "purple";
        case Accent::Custom1: return "custom1";
        case Accent::Custom2: return "custom2";
        default: return "cyan";
    }
}

Accent parseAccent(const std::string& value, Accent fallback) {
    const std::string v = lower(value);
    if (v == "cyan") return Accent::Cyan;
    if (v == "blue") return Accent::Blue;
    if (v == "green") return Accent::Green;
    if (v == "orange") return Accent::Orange;
    if (v == "pink") return Accent::Pink;
    if (v == "purple") return Accent::Purple;
    if (v == "custom1") return Accent::Custom1;
    if (v == "custom2") return Accent::Custom2;
    return fallback;
}

Accent nextAccent(Accent accent) {
    const auto next = (static_cast<uint8_t>(accent) + 1U) % static_cast<uint8_t>(Accent::Count);
    return static_cast<Accent>(next);
}

uint32_t accentRgb(Accent accent, uint32_t custom1Rgb, uint32_t custom2Rgb) {
    switch (accent) {
        case Accent::Cyan: return 0x00DCE6;
        case Accent::Blue: return 0x4687FF;
        case Accent::Green: return 0x46DC78;
        case Accent::Orange: return 0xFF9B37;
        case Accent::Pink: return 0xFF64AF;
        case Accent::Purple: return 0xAF69FF;
        case Accent::Custom1: return custom1Rgb & 0xFFFFFFU;
        case Accent::Custom2: return custom2Rgb & 0xFFFFFFU;
        default: return 0x00DCE6;
    }
}

bool parseRgbHex(const std::string& value, uint32_t& rgb) {
    size_t start = value.size() == 7 && value[0] == '#' ? 1 : 0;
    if (value.size() - start != 6) return false;
    uint32_t parsed = 0;
    for (size_t i = start; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        uint8_t digit = 0;
        if (c >= '0' && c <= '9') digit = static_cast<uint8_t>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<uint8_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<uint8_t>(c - 'A' + 10);
        else return false;
        parsed = (parsed << 4U) | digit;
    }
    rgb = parsed;
    return true;
}

std::string rgbHex(uint32_t rgb) {
    char output[8];
    std::snprintf(output, sizeof(output), "#%06X", static_cast<unsigned>(rgb & 0xFFFFFFU));
    return output;
}

ButtonAction classifyRelease(uint32_t heldMs, uint32_t tapMaxMs, uint32_t holdMaxMs) {
    if (heldMs == 0) return ButtonAction::None;
    if (heldMs < tapMaxMs) return ButtonAction::Tap;
    if (heldMs < holdMaxMs) return ButtonAction::Hold;
    return ButtonAction::LongHold;
}

bool validGroupCode(const std::string& code) {
    static const std::string allowed = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    if (code.size() != 6) return false;
    return std::all_of(code.begin(), code.end(), [&](unsigned char c) {
        return allowed.find(static_cast<char>(std::toupper(c))) != std::string::npos;
    });
}

bool validGroupPassword(const std::string& password) {
    return password.size() == 6 &&
           std::all_of(password.begin(), password.end(), [](unsigned char c) { return std::isdigit(c); });
}

int compareVersions(const std::string& a, const std::string& b) {
    std::array<uint32_t, 3> av{}, bv{};
    const bool aValid = parseVersion(a, av);
    const bool bValid = parseVersion(b, bv);
    if (aValid != bValid) return aValid ? 1 : -1;
    if (!aValid) return 0;
    for (size_t i = 0; i < av.size(); ++i) {
        if (av[i] < bv[i]) return -1;
        if (av[i] > bv[i]) return 1;
    }
    return 0;
}

bool isNewerVersion(const std::string& candidate, const std::string& current) {
    return compareVersions(candidate, current) > 0;
}

size_t selectReplacementSlot(const std::vector<SlotMeta>& slots) {
    if (slots.empty()) return 0;
    for (size_t i = 0; i < slots.size(); ++i) {
        if (!slots[i].used) return i;
    }

    size_t oldestRead = slots.size();
    uint32_t oldestReadSeq = std::numeric_limits<uint32_t>::max();
    for (size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].read && slots[i].sequence < oldestReadSeq) {
            oldestRead = i;
            oldestReadSeq = slots[i].sequence;
        }
    }
    if (oldestRead != slots.size()) return oldestRead;

    size_t oldest = 0;
    for (size_t i = 1; i < slots.size(); ++i) {
        if (slots[i].sequence < slots[oldest].sequence) oldest = i;
    }
    return oldest;
}

}  // namespace friendbox::core
