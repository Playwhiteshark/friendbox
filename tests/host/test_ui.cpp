#include "Ui.h"

#include <cassert>
#include <iostream>

using friendbox::core::ButtonAction;
using namespace friendbox::ui;

int main() {
    Ui ui;
    ui.begin();
    const NavigationContext context{3, 1, 5};

    assert(ui.screen() == Screen::Idle);
    assert(ui.handleAction(ButtonAction::Tap, context).type == IntentType::None);
    assert(ui.screen() == Screen::Inbox);
    assert(ui.inboxIndex() == 1);

    ui.handleAction(ButtonAction::Tap, context);
    assert(ui.inboxIndex() == 2);
    Intent intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::MarkInboxRead && intent.index == 2);
    ui.handleAction(ButtonAction::LongHold, context);
    assert(ui.screen() == Screen::Idle);

    ui.handleAction(ButtonAction::Hold, context);
    assert(ui.screen() == Screen::Send && ui.sendIndex() == 0);
    ui.handleAction(ButtonAction::Tap, context);
    assert(ui.sendIndex() == 1);
    intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::SendPreset && intent.index == 1);
    ui.handleAction(ButtonAction::LongHold, context);

    ui.handleAction(ButtonAction::LongHold, context);
    assert(ui.screen() == Screen::Info && ui.infoPage() == 0);
    ui.handleAction(ButtonAction::Tap, context);
    assert(ui.infoPage() == 1);
    intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::CycleAccent);

    ui.notice("SENT", "HELLO", 100, 50);
    assert(ui.noticeActive());
    ui.update(149);
    assert(ui.noticeActive());
    ui.update(150);
    assert(!ui.noticeActive());

    assert(ui.consumeDirty());
    assert(!ui.consumeDirty());
    std::cout << "UI navigation tests passed\n";
    return 0;
}
