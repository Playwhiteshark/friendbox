#include "UiRenderer.h"

#include "BuildConfig.h"
#include "FriendBoxCore.h"

namespace friendbox::ui {
namespace {

void renderIdle(display::Display& display, const RenderContext& context) {
    display.idle(context.clock, context.messages.unreadCount(),
                 context.mqttConnected ? "CONNECTED" : context.wifi,
                 context.settings.accent);
}

void renderInbox(const Ui& ui, display::Display& display, const RenderContext& context) {
    if (context.messages.count() == 0) {
        display.notification("INBOX", "No messages yet", context.settings.accent);
        return;
    }

    const size_t index = ui.inboxIndex() % context.messages.count();
    const auto* item = context.messages.at(index);
    if (!item) return;
    display.inbox(item->message.sender, item->message.text, context.selectedMessageTime,
                  index, context.messages.count(), item->unread, context.settings.accent);
}

void renderSend(const Ui& ui, display::Display& display, const RenderContext& context) {
    const auto& items = context.presets.items();
    display.sendMenu(items.data(), items.size(), ui.sendIndex(),
                     context.mqttConnected, context.settings.accent);
}

void renderInfo(const Ui& ui, display::Display& display, const RenderContext& context) {
    display.info(ui.infoPage(), context.settings.displayName,
                 context.settings.groupCode, context.settings.groupPassword,
                 context.wifi, context.mqtt, core::accentName(context.settings.accent),
                 FRIEND_BOX_VERSION, context.settings.accent);
}

}  // namespace

void render(Ui& ui, display::Display& display, const RenderContext& context) {
    if (!ui.consumeDirty()) return;
    if (ui.noticeActive()) {
        display.notification(String(ui.noticeTitle().c_str()), String(ui.noticeBody().c_str()),
                             context.settings.accent);
        return;
    }

    switch (ui.screen()) {
        case Screen::Idle: renderIdle(display, context); break;
        case Screen::Inbox: renderInbox(ui, display, context); break;
        case Screen::Send: renderSend(ui, display, context); break;
        case Screen::Info: renderInfo(ui, display, context); break;
    }
}

}  // namespace friendbox::ui
