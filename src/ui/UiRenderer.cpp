#include "UiRenderer.h"

#include <array>
#include "BuildConfig.h"
#include "FriendBoxCore.h"

namespace friendbox::ui {
namespace {

uint32_t activeAccent(const RenderContext& context) {
    return core::accentRgb(context.settings.accent, context.settings.customColor1,
                           context.settings.customColor2);
}

void renderIdle(display::Display& display, const RenderContext& context) {
    display.idle(context.settings.clockVisible ? context.clock : String(),
                 context.messages.unreadCount(),
                 context.mqttConnected ? "CONNECTED" : context.wifi,
                 activeAccent(context));
}

void renderInbox(const Ui& ui, display::Display& display, const RenderContext& context) {
    if (context.messages.count() == 0) {
        display.notification("INBOX", "No messages yet", activeAccent(context));
        return;
    }

    const size_t index = ui.inboxIndex() % context.messages.count();
    const auto* item = context.messages.at(index);
    if (!item) return;
    display.inbox(item->message.sender, item->message.text, context.selectedMessageTime,
                  index, context.messages.count(), item->unread, activeAccent(context));
}

void renderSend(const Ui& ui, display::Display& display, const RenderContext& context) {
    std::array<std::string, core::PresetCatalog::kCapacity + 1> items;
    const size_t presetCount = context.presets.enabledCount();
    for (size_t i = 0; i < presetCount; ++i) {
        const auto* item = context.presets.enabledAt(i);
        if (item) items[i] = *item;
    }
    items[presetCount] = "WRITE MESSAGE";
    display.sendMenu(items.data(), presetCount + 1, ui.sendIndex(),
                     context.mqttConnected, activeAccent(context));
}

void renderMore(const Ui& ui, display::Display& display, const RenderContext& context) {
    const std::array<std::string, 2> items{{"EDIT PRESETS", "INFO"}};
    display.selectionMenu("MORE", items.data(), items.size(), ui.moreIndex(),
                          "tap next   hold open   long back", activeAccent(context));
}

void renderPresetSelect(const Ui& ui, display::Display& display,
                        const RenderContext& context) {
    std::array<std::string, core::PresetCatalog::kCapacity> items;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto* preset = context.presets.at(i);
        items[i] = std::to_string(i + 1) + "  " +
                   (preset && !preset->empty() ? *preset : std::string("(disabled)"));
    }
    display.selectionMenu("EDIT PRESET", items.data(), items.size(), ui.presetIndex(),
                          "tap next   hold edit   long back", activeAccent(context));
}

void renderComposer(const Ui& ui, display::Display& display, const RenderContext& context) {
    const auto& composer = ui.composer();
    const String heading = ui.composeMode() == ComposeMode::Message
                               ? String("WRITE MESSAGE")
                               : String("EDIT PRESET ") + String(ui.presetIndex() + 1);
    display.composer(heading, composer.text(), composer.currentCode(),
                     composer.currentPreview(), composer.invalidCode(), composer.full(),
                     activeAccent(context));
}

void renderComposerMenu(const Ui& ui, display::Display& display,
                        const RenderContext& context) {
    std::array<std::string, 5> items{{
        ui.composeMode() == ComposeMode::Message ? "SEND" : "SAVE",
        "BACKSPACE", "SPACE", "CLEAR", "CANCEL"
    }};
    display.selectionMenu("MORSE MENU", items.data(), items.size(), ui.composerMenuIndex(),
                          "tap next   hold choose   long resume", activeAccent(context));
}

void renderInfo(const Ui& ui, display::Display& display, const RenderContext& context) {
    display.info(ui.infoPage(), context.settings.displayName,
                 context.settings.groupCode, context.settings.groupPassword,
                 context.wifi, context.mqtt, context.ota, context.otaDetail,
                 core::accentName(context.settings.accent),
                 FRIEND_BOX_VERSION, activeAccent(context));
}

}  // namespace

void render(Ui& ui, display::Display& display, const RenderContext& context) {
    if (!ui.consumeDirty()) return;
    if (ui.noticeActive()) {
        display.notification(String(ui.noticeTitle().c_str()), String(ui.noticeBody().c_str()),
                             activeAccent(context));
        return;
    }

    switch (ui.screen()) {
        case Screen::Idle: renderIdle(display, context); break;
        case Screen::Inbox: renderInbox(ui, display, context); break;
        case Screen::Send: renderSend(ui, display, context); break;
        case Screen::More: renderMore(ui, display, context); break;
        case Screen::Info: renderInfo(ui, display, context); break;
        case Screen::PresetSelect: renderPresetSelect(ui, display, context); break;
        case Screen::Composer: renderComposer(ui, display, context); break;
        case Screen::ComposerMenu: renderComposerMenu(ui, display, context); break;
    }
}

}  // namespace friendbox::ui
