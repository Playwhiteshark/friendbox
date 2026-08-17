#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace friendbox::core {

class PresetCatalog {
public:
    static constexpr size_t kCapacity = 5;
    static constexpr size_t kMaxTextLength = 96;
    using Items = std::array<std::string, kCapacity>;

    PresetCatalog();

    size_t count() const { return _items.size(); }
    size_t enabledCount() const;
    const std::string* at(size_t index) const;
    const std::string* enabledAt(size_t enabledIndex) const;
    size_t enabledSlotAt(size_t enabledIndex) const;
    const Items& items() const { return _items; }

    void load(const Items& items);
    bool replace(size_t index, std::string text);

private:
    Items _items;
};

}  // namespace friendbox::core
