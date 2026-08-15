#pragma once

#include <Arduino.h>
#include "display/Display.h"
#include "messaging/MessageStore.h"
#include "config/DeviceConfig.h"

namespace friendbox::ui {

enum class Screen { Idle, Inbox, Send, Info };

class Ui {
public:
    void begin();
    void openIdle();
    void openInbox(const messaging::MessageStore& store);
    void openSend();
    void openInfo();
    Screen screen() const { return _screen; }

    void nextInbox(const messaging::MessageStore& store);
    void nextSend();
    void nextInfo();
    size_t inboxIndex() const { return _inboxIndex; }
    size_t sendIndex() const { return _sendIndex; }
    size_t infoPage() const { return _infoPage; }
    const char* selectedPreset() const;

    void notice(const String& title, const String& body, uint32_t durationMs = 2200);
    void update();
    bool dirty() const { return _dirty; }
    void markDirty() { _dirty = true; }
    void render(display::Display& display,
                const config::Settings& settings,
                const messaging::MessageStore& store,
                const String& clock,
                const String& wifi,
                const String& mqtt,
                bool mqttConnected,
                const String& messageWhen);

    static constexpr size_t presetCount() { return 5; }

private:
    Screen _screen{Screen::Idle};
    size_t _inboxIndex{0};
    size_t _sendIndex{0};
    size_t _infoPage{0};
    bool _dirty{true};
    String _noticeTitle;
    String _noticeBody;
    uint32_t _noticeUntil{0};
};

}  // namespace friendbox::ui
