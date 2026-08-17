#include "PresetCatalog.h"

#include <utility>

namespace friendbox::core {

PresetCatalog::PresetCatalog()
    : _items{{"HELLO", "POKE", "MISS YOU", "CALL ME", "GOOD NIGHT"}} {}

const std::string* PresetCatalog::at(size_t index) const {
    return index < _items.size() ? &_items[index] : nullptr;
}

bool PresetCatalog::replace(size_t index, std::string text) {
    if (index >= _items.size() || text.empty()) return false;
    _items[index] = std::move(text);
    return true;
}

}  // namespace friendbox::core
