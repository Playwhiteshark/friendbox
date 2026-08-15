#include "FriendBoxCore.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace friendbox::core;

int main() {
    assert(parseAccent("purple") == Accent::Purple);
    assert(parseAccent("PURPLE") == Accent::Purple);
    assert(nextAccent(Accent::Purple) == Accent::Cyan);

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

    std::cout << "FriendBoxCore tests passed\n";
    return 0;
}
