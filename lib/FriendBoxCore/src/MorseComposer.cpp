#include "MorseComposer.h"

#include <algorithm>

namespace friendbox::core {
namespace {

struct MorseEntry {
    const char* code;
    char value;
};

constexpr MorseEntry kMorseTable[] = {
    {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
    {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
    {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'},
    {"----.", '9'}, {".-.-.-", '.'}, {"--..--", ','}, {"..--..", '?'},
    {"-.-.--", '!'}, {".----.", '\''}, {"-..-.", '/'}, {"-....-", '-'},
    {"-.--.", '('}, {"-.--.-", ')'}, {"---...", ':'}, {".--.-.", '@'},
};

}  // namespace

char decodeMorse(const std::string& code) {
    for (const auto& entry : kMorseTable) {
        if (code == entry.code) return entry.value;
    }
    return '\0';
}

void MorseComposer::begin(std::string text, MorseTiming timing, size_t maxLength) {
    _timing = timing;
    _maxLength = maxLength == 0 ? 1 : maxLength;
    if (text.size() > _maxLength) text.resize(_maxLength);
    _text = std::move(text);
    _currentCode.clear();
    _lastReleaseAt = 0;
    _hasRelease = false;
    _letterCommitted = false;
    _wordCommitted = false;
    _invalidCode = false;
    _full = false;
}

MorseReleaseResult MorseComposer::handleRelease(uint32_t heldMs, uint32_t releasedAtMs) {
    if (heldMs == 0) return MorseReleaseResult::Ignored;

    const uint32_t pressedAt = releasedAtMs - heldMs;
    applyGaps(pressedAt);

    if (heldMs >= _timing.controlHoldMs) {
        commitCurrent();
        // The menu intentionally ends the current Morse timing sequence. If
        // gap tracking remained armed, time spent choosing a command could be
        // mistaken for a word gap when the composer resumes.
        _hasRelease = false;
        _letterCommitted = false;
        _wordCommitted = false;
        return MorseReleaseResult::ControlRequested;
    }

    if (_currentCode.size() >= kMaxCodeLength) {
        _invalidCode = true;
        return MorseReleaseResult::Ignored;
    }

    _currentCode.push_back(heldMs < _timing.dashThresholdMs ? '.' : '-');
    _lastReleaseAt = releasedAtMs;
    _hasRelease = true;
    _letterCommitted = false;
    _wordCommitted = false;
    _invalidCode = false;
    _full = false;
    return MorseReleaseResult::SymbolAdded;
}

bool MorseComposer::update(uint32_t nowMs, bool buttonPressed) {
    if (buttonPressed) return false;
    return applyGaps(nowMs);
}

bool MorseComposer::applyGaps(uint32_t atMs) {
    if (!_hasRelease) return false;
    const uint32_t elapsed = atMs - _lastReleaseAt;
    bool changed = false;

    if (!_letterCommitted && elapsed >= _timing.letterGapMs) {
        changed |= commitCurrent();
        _letterCommitted = true;
    }
    if (!_wordCommitted && elapsed >= _timing.wordGapMs) {
        changed |= appendSpace();
        _wordCommitted = true;
    }
    return changed;
}

bool MorseComposer::appendCharacter(char value) {
    if (value == '\0') {
        _invalidCode = true;
        return false;
    }
    if (_text.size() >= _maxLength) {
        _full = true;
        return false;
    }
    _text.push_back(value);
    _invalidCode = false;
    _full = false;
    return true;
}

bool MorseComposer::commitCurrent() {
    if (_currentCode.empty()) return false;
    const char decoded = decodeMorse(_currentCode);
    _currentCode.clear();
    appendCharacter(decoded);
    return true;
}

bool MorseComposer::backspace() {
    _invalidCode = false;
    _full = false;
    if (!_currentCode.empty()) {
        _currentCode.pop_back();
        return true;
    }
    if (_text.empty()) return false;
    _text.pop_back();
    return true;
}

bool MorseComposer::appendSpace() {
    _invalidCode = false;
    if (_text.empty() || _text.back() == ' ') return false;
    if (_text.size() >= _maxLength) {
        _full = true;
        return false;
    }
    _text.push_back(' ');
    _full = false;
    return true;
}

bool MorseComposer::clear() {
    const bool changed = !_text.empty() || !_currentCode.empty();
    _text.clear();
    _currentCode.clear();
    _invalidCode = false;
    _full = false;
    _hasRelease = false;
    _letterCommitted = false;
    _wordCommitted = false;
    return changed;
}

char MorseComposer::currentPreview() const {
    return decodeMorse(_currentCode);
}

}  // namespace friendbox::core
