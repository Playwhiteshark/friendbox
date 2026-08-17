#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace friendbox::core {

class PresetCatalog {
public:
    static constexpr size_t kCapacity = 5;
    using Items = std::array<std::string, kCapacity>;

    PresetCatalog();

    size_t count() const { return _items.size(); }
    const std::string* at(size_t index) const;
    const Items& items() const { return _items; }

    // Persistence and editing UIs will call this later. Keeping mutation here
    // prevents the send screen from owning configuration policy.
    bool replace(size_t index, std::string text);

private:
    Items _items;
};

}  // namespace friendbox::core
