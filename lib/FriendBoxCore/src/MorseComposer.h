#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace friendbox::core {

struct MorseTiming {
    uint32_t dashThresholdMs{300};
    uint32_t letterGapMs{700};
    uint32_t wordGapMs{1500};
    uint32_t controlHoldMs{1600};
};

enum class MorseReleaseResult : uint8_t {
    Ignored,
    SymbolAdded,
    ControlRequested,
};

class MorseComposer {
public:
    static constexpr size_t kDefaultMaxLength = 160;
    static constexpr size_t kMaxCodeLength = 6;

    void begin(std::string text = {}, MorseTiming timing = {},
               size_t maxLength = kDefaultMaxLength);
    MorseReleaseResult handleRelease(uint32_t heldMs, uint32_t releasedAtMs);
    bool update(uint32_t nowMs, bool buttonPressed = false);

    bool backspace();
    bool appendSpace();
    bool clear();
    bool commitCurrent();

    const std::string& text() const { return _text; }
    const std::string& currentCode() const { return _currentCode; }
    char currentPreview() const;
    bool invalidCode() const { return _invalidCode; }
    bool full() const { return _full; }
    size_t maxLength() const { return _maxLength; }

private:
    std::string _text;
    std::string _currentCode;
    MorseTiming _timing;
    size_t _maxLength{kDefaultMaxLength};
    uint32_t _lastReleaseAt{0};
    bool _hasRelease{false};
    bool _letterCommitted{false};
    bool _wordCommitted{false};
    bool _invalidCode{false};
    bool _full{false};

    bool applyGaps(uint32_t atMs);
    bool appendCharacter(char value);
};

char decodeMorse(const std::string& code);

}  // namespace friendbox::core
