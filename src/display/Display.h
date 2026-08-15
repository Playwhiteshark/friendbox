#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "FriendBoxCore.h"

namespace friendbox::display {

class Display {
public:
    bool begin();
    void clear();
    uint16_t accentColor(core::Accent accent) const;

    void boot(const String& line = "starting...");
    void idle(const String& timeText, size_t unread, const String& network, core::Accent accent);
    void inbox(const String& sender, const String& text, const String& when,
               size_t position, size_t count, bool unread, core::Accent accent);
    void sendMenu(const char* const* items, size_t count, size_t selected, bool connected, core::Accent accent);
    void info(size_t page, const String& name, const String& groupCode, const String& groupPassword,
              const String& wifi, const String& mqtt, const String& accentName,
              const String& version, core::Accent accent);
    void notification(const String& title, const String& body, core::Accent accent);
    void setupMode(const String& apName);
    void fatal(const String& message);

private:
    TFT_eSPI _gfx;

    void title(const String& text, uint16_t accent);
    void footer(const String& text);
    void centered(const String& text, int16_t y, uint8_t size, uint16_t color);
    void wrapped(const String& text, int16_t x, int16_t y, int16_t maxWidth, uint8_t size, uint16_t color, uint8_t maxLines = 3);
};

}  // namespace friendbox::display
