#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <string>
#include "FriendBoxCore.h"

namespace friendbox::display {

struct Area {
    int16_t x{0};
    int16_t y{0};
    int16_t width{0};
    int16_t height{0};
};

struct DetailRow {
    String label;
    String value;
};

class Display {
public:
    bool begin();
    void clear();
    uint16_t accentColor(uint32_t rgb) const;

    void boot(const String& line = "starting...");
    void idle(const String& timeText, size_t unread, const String& network, uint32_t accentRgb);
    void inbox(const String& sender, const String& text, const String& when,
               size_t position, size_t count, bool unread, uint32_t accentRgb);
    void sendMenu(const std::string* items, size_t count, size_t selected,
                  bool connected, uint32_t accentRgb);
    void selectionMenu(const String& heading, const std::string* items, size_t count,
                       size_t selected, const String& help, uint32_t accentRgb);
    void composer(const String& heading, const std::string& text, const std::string& code,
                  char preview, bool invalid, bool full, uint32_t accentRgb);
    void info(size_t page, const String& name, const String& groupCode, const String& groupPassword,
              const String& wifi, const String& mqtt, const String& ota,
              const String& otaDetail, const String& accentName, const String& version,
              uint32_t accentRgb);
    void notification(const String& title, const String& body, uint32_t accentRgb);
    void setupMode(const String& apName);
    void fatal(const String& message);

private:
    TFT_eSPI _gfx;

    void title(const String& text, uint16_t accent);
    void footer(const String& text);
    void drawText(const Area& area, const String& text, uint8_t size, uint16_t color);
    void centered(const Area& area, const String& text, uint8_t size, uint16_t color);
    void wrapped(const Area& area, const String& text, uint8_t size, uint16_t color,
                 uint8_t maxLines = 3);
    void drawClock(const Area& area, const String& timeText, uint8_t size, uint16_t color);
    void drawPageIndicator(const Area& area, size_t position, size_t count, uint16_t color);
    void drawMenuList(const Area& area, const std::string* items, size_t count,
                      size_t selected, uint16_t accent);
    void drawDetailList(const Area& area, const DetailRow* rows, size_t count,
                        uint8_t size, uint16_t color, int16_t rowHeight = 17);
};

}  // namespace friendbox::display
