#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "FriendBoxCore.h"

namespace friendbox::ui {

enum class Screen : uint8_t { Idle, Inbox, Send, Info };

enum class IntentType : uint8_t {
    None,
    MarkInboxRead,
    SendPreset,
    CycleAccent,
};

struct Intent {
    IntentType type{IntentType::None};
    size_t index{0};
};

struct NavigationContext {
    size_t inboxCount{0};
    size_t firstUnreadIndex{0};
    size_t sendItemCount{0};
};

class Ui {
public:
    void begin();
    Intent handleAction(core::ButtonAction action, const NavigationContext& context);

    Screen screen() const { return _screen; }
    size_t inboxIndex() const { return _inboxIndex; }
    size_t sendIndex() const { return _sendIndex; }
    size_t infoPage() const { return _infoPage; }

    void notice(std::string title, std::string body, uint32_t nowMs,
                uint32_t durationMs = 2200);
    void update(uint32_t nowMs);
    bool noticeActive() const { return _noticeActive; }
    const std::string& noticeTitle() const { return _noticeTitle; }
    const std::string& noticeBody() const { return _noticeBody; }

    bool consumeDirty();
    void markDirty() { _dirty = true; }

private:
    static constexpr size_t kInfoPageCount = 3;

    Screen _screen{Screen::Idle};
    size_t _inboxIndex{0};
    size_t _sendIndex{0};
    size_t _infoPage{0};
    bool _dirty{true};
    bool _noticeActive{false};
    std::string _noticeTitle;
    std::string _noticeBody;
    uint32_t _noticeUntil{0};

    Intent handleIdleAction(core::ButtonAction action, const NavigationContext& context);
    Intent handleInboxAction(core::ButtonAction action, const NavigationContext& context);
    Intent handleSendAction(core::ButtonAction action, const NavigationContext& context);
    Intent handleInfoAction(core::ButtonAction action);
    void open(Screen screen);
};

}  // namespace friendbox::ui
