#include "Ui.h"

#include <utility>

namespace friendbox::ui {

void Ui::begin() {
    _screen = Screen::Idle;
    _inboxIndex = 0;
    _sendIndex = 0;
    _infoPage = 0;
    _noticeActive = false;
    _dirty = true;
}

Intent Ui::handleAction(core::ButtonAction action, const NavigationContext& context) {
    if (action == core::ButtonAction::None) return {};
    switch (_screen) {
        case Screen::Idle: return handleIdleAction(action, context);
        case Screen::Inbox: return handleInboxAction(action, context);
        case Screen::Send: return handleSendAction(action, context);
        case Screen::Info: return handleInfoAction(action);
    }
    return {};
}

Intent Ui::handleIdleAction(core::ButtonAction action, const NavigationContext& context) {
    if (action == core::ButtonAction::Tap) {
        _inboxIndex = context.inboxCount == 0 ? 0 : context.firstUnreadIndex % context.inboxCount;
        open(Screen::Inbox);
    } else if (action == core::ButtonAction::Hold) {
        _sendIndex = 0;
        open(Screen::Send);
    } else if (action == core::ButtonAction::LongHold) {
        _infoPage = 0;
        open(Screen::Info);
    }
    return {};
}

Intent Ui::handleInboxAction(core::ButtonAction action, const NavigationContext& context) {
    if (action == core::ButtonAction::Tap) {
        if (context.inboxCount > 0) _inboxIndex = (_inboxIndex + 1) % context.inboxCount;
        _dirty = true;
        return {};
    }
    if (action == core::ButtonAction::Hold) return {IntentType::MarkInboxRead, _inboxIndex};
    if (action == core::ButtonAction::LongHold) open(Screen::Idle);
    return {};
}

Intent Ui::handleSendAction(core::ButtonAction action, const NavigationContext& context) {
    if (action == core::ButtonAction::Tap) {
        if (context.sendItemCount > 0) _sendIndex = (_sendIndex + 1) % context.sendItemCount;
        _dirty = true;
        return {};
    }
    if (action == core::ButtonAction::Hold) return {IntentType::SendPreset, _sendIndex};
    if (action == core::ButtonAction::LongHold) open(Screen::Idle);
    return {};
}

Intent Ui::handleInfoAction(core::ButtonAction action) {
    if (action == core::ButtonAction::Tap) {
        _infoPage = (_infoPage + 1) % kInfoPageCount;
        _dirty = true;
        return {};
    }
    if (action == core::ButtonAction::Hold) return {IntentType::CycleAccent, 0};
    if (action == core::ButtonAction::LongHold) open(Screen::Idle);
    return {};
}

void Ui::open(Screen screen) {
    _screen = screen;
    _dirty = true;
}

void Ui::notice(std::string title, std::string body, uint32_t nowMs, uint32_t durationMs) {
    _noticeTitle = std::move(title);
    _noticeBody = std::move(body);
    _noticeUntil = nowMs + durationMs;
    _noticeActive = true;
    _dirty = true;
}

void Ui::update(uint32_t nowMs) {
    if (_noticeActive && static_cast<int32_t>(nowMs - _noticeUntil) >= 0) {
        _noticeActive = false;
        _noticeTitle.clear();
        _noticeBody.clear();
        _dirty = true;
    }
}

bool Ui::consumeDirty() {
    if (!_dirty) return false;
    _dirty = false;
    return true;
}

}  // namespace friendbox::ui
