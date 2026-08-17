#include "Ui.h"

#include <cassert>
#include <iostream>

using friendbox::core::ButtonAction;
using namespace friendbox::ui;

int main() {
    const NavigationContext context{3, 1, 3};

    Ui ui;
    ui.begin();
    assert(ui.screen() == Screen::Idle);
    assert(ui.handleAction(ButtonAction::Tap, context).type == IntentType::None);
    assert(ui.screen() == Screen::Inbox && ui.inboxIndex() == 1);
    ui.handleAction(ButtonAction::Tap, context);
    Intent intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::MarkInboxRead && intent.index == 2);
    ui.handleAction(ButtonAction::LongHold, context);

    ui.handleAction(ButtonAction::Hold, context);
    assert(ui.screen() == Screen::Send && ui.sendIndex() == 0);
    ui.handleAction(ButtonAction::Tap, context);
    intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::SendPreset && intent.index == 1);
    ui.handleAction(ButtonAction::Tap, context);
    ui.handleAction(ButtonAction::Tap, context);
    intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::BeginMessage);

    ui.startMessageComposer({});
    assert(ui.screen() == Screen::Composer && ui.morseEntryActive());
    ui.handleMorseRelease(100, 100);
    ui.handleMorseRelease(350, 600);
    assert(ui.composer().currentPreview() == 'A');
    ui.handleMorseRelease(1600, 3000);
    assert(ui.screen() == Screen::ComposerMenu && ui.composedText() == "A");
    intent = ui.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::SendComposed);
    assert(ui.screen() == Screen::Send);

    Ui settingsUi;
    settingsUi.begin();
    settingsUi.handleAction(ButtonAction::LongHold, context);
    assert(settingsUi.screen() == Screen::More && settingsUi.moreIndex() == 0);
    settingsUi.handleAction(ButtonAction::Hold, context);
    assert(settingsUi.screen() == Screen::PresetSelect);
    settingsUi.handleAction(ButtonAction::Tap, context);
    intent = settingsUi.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::BeginPresetEdit && intent.index == 1);

    settingsUi.startPresetComposer(1, "OLD", {});
    settingsUi.handleMorseRelease(1600, 2000);
    assert(settingsUi.screen() == Screen::ComposerMenu);
    settingsUi.handleAction(ButtonAction::Tap, context);
    settingsUi.handleAction(ButtonAction::Hold, context);
    assert(settingsUi.screen() == Screen::Composer && settingsUi.composedText() == "OL");
    settingsUi.handleMorseRelease(1600, 4000);
    intent = settingsUi.handleAction(ButtonAction::Hold, context);
    assert(intent.type == IntentType::SavePreset && intent.index == 1);
    assert(settingsUi.screen() == Screen::PresetSelect);

    settingsUi.handleAction(ButtonAction::LongHold, context);
    assert(settingsUi.screen() == Screen::More);
    settingsUi.handleAction(ButtonAction::Tap, context);
    settingsUi.handleAction(ButtonAction::Hold, context);
    assert(settingsUi.screen() == Screen::Info);
    settingsUi.handleAction(ButtonAction::Tap, context);
    assert(settingsUi.infoPage() == 1);
    settingsUi.handleAction(ButtonAction::LongHold, context);
    assert(settingsUi.screen() == Screen::More);

    settingsUi.notice("SENT", "HELLO", 100, 50);
    assert(settingsUi.noticeActive());
    settingsUi.update(149);
    assert(settingsUi.noticeActive());
    settingsUi.update(150);
    assert(!settingsUi.noticeActive());

    assert(settingsUi.consumeDirty());
    assert(!settingsUi.consumeDirty());
    std::cout << "UI navigation and Morse composition tests passed\n";
    return 0;
}
