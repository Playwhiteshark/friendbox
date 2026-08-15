#include "Ui.h"
#include "BuildConfig.h"
#include "FriendBoxCore.h"

namespace friendbox::ui {
namespace {
const char* const kPresets[] = {"HELLO", "POKE", "MISS YOU", "CALL ME", "GOOD NIGHT"};
}

void Ui::begin() { _dirty = true; }
void Ui::openIdle() { _screen = Screen::Idle; _dirty = true; }
void Ui::openInbox(const messaging::MessageStore& store) {
    _screen = Screen::Inbox;
    _inboxIndex = store.firstUnreadIndex();
    _dirty = true;
}
void Ui::openSend() { _screen = Screen::Send; _sendIndex = 0; _dirty = true; }
void Ui::openInfo() { _screen = Screen::Info; _infoPage = 0; _dirty = true; }

void Ui::nextInbox(const messaging::MessageStore& store) {
    if (store.count()) _inboxIndex = (_inboxIndex + 1) % store.count();
    _dirty = true;
}
void Ui::nextSend() { _sendIndex = (_sendIndex + 1) % presetCount(); _dirty = true; }
void Ui::nextInfo() { _infoPage = (_infoPage + 1) % 3; _dirty = true; }
const char* Ui::selectedPreset() const { return kPresets[_sendIndex % presetCount()]; }

void Ui::notice(const String& title, const String& body, uint32_t durationMs) {
    _noticeTitle = title;
    _noticeBody = body;
    _noticeUntil = millis() + durationMs;
    _dirty = true;
}

void Ui::update() {
    if (_noticeUntil != 0 && static_cast<int32_t>(millis() - _noticeUntil) >= 0) {
        _noticeUntil = 0;
        _noticeTitle = "";
        _noticeBody = "";
        _dirty = true;
    }
}

void Ui::render(display::Display& display,
                const config::Settings& settings,
                const messaging::MessageStore& store,
                const String& clock,
                const String& wifi,
                const String& mqtt,
                bool mqttConnected,
                const String& messageWhen) {
    if (!_dirty) return;
    _dirty = false;
    if (_noticeUntil != 0) {
        display.notification(_noticeTitle, _noticeBody, settings.accent);
        return;
    }

    switch (_screen) {
        case Screen::Idle:
            display.idle(clock, store.unreadCount(), mqttConnected ? "CONNECTED" : wifi, settings.accent);
            break;
        case Screen::Inbox: {
            if (store.count() == 0) {
                display.notification("INBOX", "No messages yet", settings.accent);
                break;
            }
            const size_t safe = _inboxIndex % store.count();
            const auto* item = store.at(safe);
            display.inbox(item->message.sender, item->message.text, messageWhen,
                          safe, store.count(), item->unread, settings.accent);
            break;
        }
        case Screen::Send:
            display.sendMenu(kPresets, presetCount(), _sendIndex, mqttConnected, settings.accent);
            break;
        case Screen::Info:
            display.info(_infoPage, settings.displayName, settings.groupCode, settings.groupPassword,
                         wifi, mqtt, core::accentName(settings.accent), FRIEND_BOX_VERSION, settings.accent);
            break;
    }
}

}  // namespace friendbox::ui
