#include "FriendBoxCore.h"
#include "MorseComposer.h"
#include "PresetCatalog.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace friendbox::core;

int main() {
    assert(parseAccent("purple") == Accent::Purple);
    assert(parseAccent("PURPLE") == Accent::Purple);
    assert(nextAccent(Accent::Purple) == Accent::Custom1);
    assert(nextAccent(Accent::Custom2) == Accent::Cyan);
    assert(parseAccent("custom1") == Accent::Custom1);
    assert(accentRgb(Accent::Custom2, 0x123456, 0xABCDEF) == 0xABCDEF);
    uint32_t rgb = 0;
    assert(parseRgbHex("#12aBcD", rgb) && rgb == 0x12ABCD);
    assert(rgbHex(rgb) == "#12ABCD");
    assert(!parseRgbHex("not-a-color", rgb));

    assert(classifyRelease(100) == ButtonAction::Tap);
    assert(classifyRelease(449) == ButtonAction::Tap);
    assert(classifyRelease(450) == ButtonAction::Hold);
    assert(classifyRelease(1199) == ButtonAction::Hold);
    assert(classifyRelease(1200) == ButtonAction::LongHold);

    assert(validGroupCode("F7K3Q2"));
    assert(validGroupCode("f7k3q2"));
    assert(!validGroupCode("O0I1AA"));
    assert(validGroupPassword("482731"));
    assert(!validGroupPassword("48273A"));

    assert(compareVersions("1.0.0", "1.0.0") == 0);
    assert(compareVersions("1.0.1", "1.0.0") > 0);
    assert(compareVersions("2.0.0", "1.99.99") > 0);
    assert(compareVersions("bad", "1.0.0") < 0);
    assert(compareVersions("1.2", "1.0.0") < 0);
    assert(compareVersions("1.2.3.4", "1.0.0") < 0);
    assert(compareVersions("4294967296.0.0", "1.0.0") < 0);
    assert(isNewerVersion("1.1.0", "1.0.9"));

    std::vector<SlotMeta> slots(3);
    slots[0] = {true, false, 1};
    slots[1] = {false, false, 0};
    slots[2] = {true, true, 2};
    assert(selectReplacementSlot(slots) == 1);

    slots[1] = {true, true, 4};
    assert(selectReplacementSlot(slots) == 2); // oldest read

    slots[2].read = false;
    slots[1].read = false;
    assert(selectReplacementSlot(slots) == 0); // oldest overall

    PresetCatalog presets;
    assert(presets.count() == PresetCatalog::kCapacity);
    assert(presets.at(0) && *presets.at(0) == "HELLO");
    assert(presets.replace(0, "ON MY WAY"));
    assert(*presets.at(0) == "ON MY WAY");
    assert(!presets.replace(PresetCatalog::kCapacity, "INVALID"));
    assert(presets.replace(0, ""));
    assert(presets.enabledCount() == 4);
    assert(presets.enabledSlotAt(0) == 1);
    assert(*presets.enabledAt(0) == "POKE");
    assert(!presets.replace(0, std::string(PresetCatalog::kMaxTextLength + 1, 'X')));

    assert(decodeMorse("...") == 'S');
    assert(decodeMorse(".----") == '1');
    assert(decodeMorse("......") == '\0');

    MorseComposer composer;
    composer.begin();
    assert(composer.handleRelease(100, 100) == MorseReleaseResult::SymbolAdded);  // .
    assert(composer.handleRelease(350, 600) == MorseReleaseResult::SymbolAdded);  // -
    assert(composer.currentPreview() == 'A');
    assert(composer.update(1299) == false);
    assert(composer.update(1300));
    assert(composer.text() == "A");
    assert(composer.update(2100));
    assert(composer.text() == "A ");
    assert(composer.handleRelease(100, 2300) == MorseReleaseResult::SymbolAdded);
    assert(composer.handleRelease(1600, 4700) == MorseReleaseResult::ControlRequested);
    assert(composer.text() == "A E");
    assert(!composer.update(10000));
    assert(composer.text() == "A E");
    assert(composer.backspace() && composer.text() == "A ");
    assert(composer.clear() && composer.text().empty());

    std::cout << "FriendBoxCore tests passed\n";
    return 0;
}
