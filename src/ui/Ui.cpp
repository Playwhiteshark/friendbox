#include "Ui.h"

#include <utility>

namespace friendbox::ui {

void Ui::begin() {
    _screen = Screen::Idle;
    _composeMode = ComposeMode::Message;
    _inboxIndex = 0;
    _sendIndex = 0;
    _moreIndex = 0;
    _presetIndex = 0;
    _composerMenuIndex = 0;
    _infoPage = 0;
    _noticeActive = false;
    _dirty = true;
}

Intent Ui::handleAction(core::ButtonAction action, const NavigationContext& context) {
    if (action == core::ButtonAction::None || _screen == Screen::Composer) return {};
    switch (_screen) {
        case Screen::Idle: return handleIdleAction(action, context);
        case Screen::Inbox: return handleInboxAction(action, context);
        case Screen::Send: return handleSendAction(action, context);
        case Screen::More: return handleMoreAction(action);
        case Screen::Info: return handleInfoAction(action);
        case Screen::PresetSelect: return handlePresetSelectAction(action);
        case Screen::ComposerMenu: return handleComposerMenuAction(action);
        case Screen::Composer: return {};
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
        _moreIndex = 0;
        open(Screen::More);
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
    const size_t itemCount = context.enabledPresetCount + 1;
    if (action == core::ButtonAction::Tap) {
        _sendIndex = itemCount == 0 ? 0 : (_sendIndex + 1) % itemCount;
        _dirty = true;
        return {};
    }
    if (action == core::ButtonAction::Hold) {
        return _sendIndex < context.enabledPresetCount
                   ? Intent{IntentType::SendPreset, _sendIndex}
                   : Intent{IntentType::BeginMessage, 0};
    }
    if (action == core::ButtonAction::LongHold) open(Screen::Idle);
    return {};
}

Intent Ui::handleMoreAction(core::ButtonAction action) {
    if (action == core::ButtonAction::Tap) {
        _moreIndex = (_moreIndex + 1) % kMoreItemCount;
        _dirty = true;
    } else if (action == core::ButtonAction::Hold) {
        if (_moreIndex == 0) {
            _presetIndex = 0;
            open(Screen::PresetSelect);
        } else {
            _infoPage = 0;
            open(Screen::Info);
        }
    } else if (action == core::ButtonAction::LongHold) {
        open(Screen::Idle);
    }
    return {};
}

Intent Ui::handleInfoAction(core::ButtonAction action) {
    if (action == core::ButtonAction::Tap) {
        _infoPage = (_infoPage + 1) % kInfoPageCount;
        _dirty = true;
    } else if (action == core::ButtonAction::LongHold) {
        open(Screen::More);
    }
    return {};
}

Intent Ui::handlePresetSelectAction(core::ButtonAction action) {
    if (action == core::ButtonAction::Tap) {
        _presetIndex = (_presetIndex + 1) % core::PresetCatalog::kCapacity;
        _dirty = true;
    } else if (action == core::ButtonAction::Hold) {
        return {IntentType::BeginPresetEdit, _presetIndex};
    } else if (action == core::ButtonAction::LongHold) {
        open(Screen::More);
    }
    return {};
}

void Ui::startMessageComposer(core::MorseTiming timing) {
    _composeMode = ComposeMode::Message;
    _composer.begin({}, timing, core::MorseComposer::kDefaultMaxLength);
    open(Screen::Composer);
}

void Ui::startPresetComposer(size_t slot, std::string text, core::MorseTiming timing) {
    _composeMode = ComposeMode::Preset;
    _presetIndex = slot % core::PresetCatalog::kCapacity;
    _composer.begin(std::move(text), timing, core::PresetCatalog::kMaxTextLength);
    open(Screen::Composer);
}

void Ui::handleMorseRelease(uint32_t heldMs, uint32_t nowMs) {
    if (_screen != Screen::Composer) return;
    const auto result = _composer.handleRelease(heldMs, nowMs);
    if (result == core::MorseReleaseResult::ControlRequested) {
        _composerMenuIndex = 0;
        open(Screen::ComposerMenu);
    } else if (result != core::MorseReleaseResult::Ignored) {
        _dirty = true;
    }
}

Intent Ui::handleComposerMenuAction(core::ButtonAction action) {
    if (action == core::ButtonAction::Tap) {
        _composerMenuIndex = (_composerMenuIndex + 1) % kComposerMenuItemCount;
        _dirty = true;
        return {};
    }
    if (action == core::ButtonAction::LongHold) {
        open(Screen::Composer);
        return {};
    }
    if (action != core::ButtonAction::Hold) return {};

    switch (_composerMenuIndex) {
        case 0:
            if (_composeMode == ComposeMode::Message && _composer.text().empty()) {
                open(Screen::Composer);
                return {};
            }
            _screen = _composeMode == ComposeMode::Message ? Screen::Send : Screen::PresetSelect;
            _dirty = true;
            return {_composeMode == ComposeMode::Message ? IntentType::SendComposed
                                                         : IntentType::SavePreset,
                    _presetIndex};
        case 1:
            _composer.backspace();
            open(Screen::Composer);
            return {};
        case 2:
            _composer.appendSpace();
            open(Screen::Composer);
            return {};
        case 3:
            _composer.clear();
            open(Screen::Composer);
            return {};
        case 4:
            leaveComposer();
            return {};
        default:
            return {};
    }
}

void Ui::leaveComposer() {
    open(_composeMode == ComposeMode::Message ? Screen::Send : Screen::PresetSelect);
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

void Ui::update(uint32_t nowMs, bool buttonPressed) {
    if (_noticeActive && static_cast<int32_t>(nowMs - _noticeUntil) >= 0) {
        _noticeActive = false;
        _noticeTitle.clear();
        _noticeBody.clear();
        _dirty = true;
    }
    if (_screen == Screen::Composer && _composer.update(nowMs, buttonPressed)) {
        _dirty = true;
    }
}

bool Ui::consumeDirty() {
    if (!_dirty) return false;
    _dirty = false;
    return true;
}

}  // namespace friendbox::ui
