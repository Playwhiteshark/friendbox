#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "FriendBoxCore.h"
#include "MorseComposer.h"
#include "PresetCatalog.h"

namespace friendbox::ui {

enum class Screen : uint8_t {
    Idle,
    Inbox,
    Send,
    More,
    Info,
    PresetSelect,
    Composer,
    ComposerMenu,
};

enum class ComposeMode : uint8_t { Message, Preset };

enum class IntentType : uint8_t {
    None,
    MarkInboxRead,
    SendPreset,
    BeginMessage,
    BeginPresetEdit,
    SendComposed,
    SavePreset,
};

struct Intent {
    IntentType type{IntentType::None};
    size_t index{0};
};

struct NavigationContext {
    size_t inboxCount{0};
    size_t firstUnreadIndex{0};
    size_t enabledPresetCount{0};
};

class Ui {
public:
    void begin();
    Intent handleAction(core::ButtonAction action, const NavigationContext& context);
    void handleMorseRelease(uint32_t heldMs, uint32_t nowMs);
    void startMessageComposer(core::MorseTiming timing);
    void startPresetComposer(size_t slot, std::string text, core::MorseTiming timing);

    Screen screen() const { return _screen; }
    size_t inboxIndex() const { return _inboxIndex; }
    size_t sendIndex() const { return _sendIndex; }
    size_t moreIndex() const { return _moreIndex; }
    size_t presetIndex() const { return _presetIndex; }
    size_t composerMenuIndex() const { return _composerMenuIndex; }
    size_t infoPage() const { return _infoPage; }
    ComposeMode composeMode() const { return _composeMode; }
    bool morseEntryActive() const { return _screen == Screen::Composer; }
    const core::MorseComposer& composer() const { return _composer; }
    const std::string& composedText() const { return _composer.text(); }

    void notice(std::string title, std::string body, uint32_t nowMs,
                uint32_t durationMs = 2200);
    void update(uint32_t nowMs, bool buttonPressed = false);
    bool noticeActive() const { return _noticeActive; }
    const std::string& noticeTitle() const { return _noticeTitle; }
    const std::string& noticeBody() const { return _noticeBody; }

    bool consumeDirty();
    void markDirty() { _dirty = true; }

private:
    static constexpr size_t kInfoPageCount = 4;
    static constexpr size_t kMoreItemCount = 2;
    static constexpr size_t kComposerMenuItemCount = 5;

    Screen _screen{Screen::Idle};
    ComposeMode _composeMode{ComposeMode::Message};
    size_t _inboxIndex{0};
    size_t _sendIndex{0};
    size_t _moreIndex{0};
    size_t _presetIndex{0};
    size_t _composerMenuIndex{0};
    size_t _infoPage{0};
    core::MorseComposer _composer;
    bool _dirty{true};
    bool _noticeActive{false};
    std::string _noticeTitle;
    std::string _noticeBody;
    uint32_t _noticeUntil{0};

    Intent handleIdleAction(core::ButtonAction action, const NavigationContext& context);
    Intent handleInboxAction(core::ButtonAction action, const NavigationContext& context);
    Intent handleSendAction(core::ButtonAction action, const NavigationContext& context);
    Intent handleMoreAction(core::ButtonAction action);
    Intent handleInfoAction(core::ButtonAction action);
    Intent handlePresetSelectAction(core::ButtonAction action);
    Intent handleComposerMenuAction(core::ButtonAction action);
    void open(Screen screen);
    void leaveComposer();
};

}  // namespace friendbox::ui
