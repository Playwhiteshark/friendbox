#include "PresetCatalog.h"

#include <utility>

namespace friendbox::core {

PresetCatalog::PresetCatalog()
    : _items{{"HELLO", "POKE", "MISS YOU", "CALL ME", "GOOD NIGHT"}} {}

const std::string* PresetCatalog::at(size_t index) const {
    return index < _items.size() ? &_items[index] : nullptr;
}

size_t PresetCatalog::enabledCount() const {
    size_t count = 0;
    for (const auto& item : _items) {
        if (!item.empty()) ++count;
    }
    return count;
}

const std::string* PresetCatalog::enabledAt(size_t enabledIndex) const {
    const size_t slot = enabledSlotAt(enabledIndex);
    return slot < _items.size() ? &_items[slot] : nullptr;
}

size_t PresetCatalog::enabledSlotAt(size_t enabledIndex) const {
    size_t current = 0;
    for (size_t slot = 0; slot < _items.size(); ++slot) {
        if (_items[slot].empty()) continue;
        if (current == enabledIndex) return slot;
        ++current;
    }
    return _items.size();
}

void PresetCatalog::load(const Items& items) {
    for (size_t i = 0; i < _items.size(); ++i) replace(i, items[i]);
}

bool PresetCatalog::replace(size_t index, std::string text) {
    if (index >= _items.size() || text.size() > kMaxTextLength) return false;
    _items[index] = std::move(text);
    return true;
}

}  // namespace friendbox::core
