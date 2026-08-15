#include "Display.h"

namespace friendbox::display {
namespace {
constexpr int16_t kWidth = 320;
}

bool Display::begin() {
    // Exact pin mapping from LILYGO's official T-Display-S3 Arduino_GFX example.
    _bus = new Arduino_ESP32PAR8Q(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
    _gfx = new Arduino_ST7789(_bus, 5, 0, true, 170, 320, 35, 0, 35, 0);
    if (!_gfx || !_gfx->begin()) return false;
    _gfx->setRotation(1);
    _gfx->setTextWrap(false);
    clear();
    return true;
}

void Display::clear() {
    if (_gfx) _gfx->fillScreen(BLACK);
}

uint16_t Display::accentColor(core::Accent accent) const {
    if (!_gfx) return WHITE;
    switch (accent) {
        case core::Accent::Cyan: return _gfx->color565(0, 220, 230);
        case core::Accent::Blue: return _gfx->color565(70, 135, 255);
        case core::Accent::Green: return _gfx->color565(70, 220, 120);
        case core::Accent::Orange: return _gfx->color565(255, 155, 55);
        case core::Accent::Pink: return _gfx->color565(255, 100, 175);
        case core::Accent::Purple: return _gfx->color565(175, 105, 255);
        default: return WHITE;
    }
}

void Display::centered(const String& text, int16_t y, uint8_t size, uint16_t color) {
    if (!_gfx) return;
    const int16_t approximateWidth = static_cast<int16_t>(text.length() * 6U * size);
    const int16_t centeredX = (kWidth - approximateWidth) / 2;
    const int16_t x = centeredX < 4 ? 4 : centeredX;
    _gfx->setTextSize(size);
    _gfx->setTextColor(color);
    _gfx->setCursor(x, y);
    _gfx->print(text);
}

void Display::title(const String& text, uint16_t accent) {
    _gfx->fillRect(0, 0, kWidth, 29, BLACK);
    _gfx->setTextSize(2);
    _gfx->setTextColor(accent);
    _gfx->setCursor(10, 8);
    _gfx->print(text);
    _gfx->drawFastHLine(10, 28, 300, accent);
}

void Display::footer(const String& text) {
    _gfx->fillRect(0, 149, kWidth, 21, BLACK);
    _gfx->setTextSize(1);
    _gfx->setTextColor(_gfx->color565(150, 150, 150));
    _gfx->setCursor(10, 156);
    _gfx->print(text);
}

void Display::wrapped(const String& text, int16_t x, int16_t y, int16_t maxWidth,
                      uint8_t size, uint16_t color, uint8_t maxLines) {
    _gfx->setTextSize(size);
    _gfx->setTextColor(color);
    const int charWidth = 6 * size;
    const size_t calculatedChars = static_cast<size_t>(maxWidth / charWidth);
    const size_t charsPerLine = calculatedChars < 1 ? 1 : calculatedChars;
    String remaining = text;
    for (uint8_t line = 0; line < maxLines && !remaining.isEmpty(); ++line) {
        size_t take = min(charsPerLine, remaining.length());
        if (take < remaining.length()) {
            int split = remaining.substring(0, take + 1).lastIndexOf(' ');
            if (split > 0) take = static_cast<size_t>(split);
        }
        String part = remaining.substring(0, take);
        part.trim();
        _gfx->setCursor(x, y + line * (8 * size + 3));
        _gfx->print(part);
        remaining = remaining.substring(min(remaining.length(), take + (take < remaining.length() && remaining[take] == ' ' ? 1 : 0)));
    }
}

void Display::boot(const String& line) {
    clear();
    centered("FriendBox", 55, 3, WHITE);
    centered(line, 96, 1, _gfx->color565(155, 155, 155));
}

void Display::idle(const String& timeText, size_t unread, const String& network, core::Accent accent) {
    clear();
    const uint16_t a = accentColor(accent);
    title("FRIENDBOX", a);
    centered(timeText, 48, 4, WHITE);
    if (unread > 0) {
        centered(String(unread) + (unread == 1 ? " NEW MESSAGE" : " NEW MESSAGES"), 91, 2, a);
    } else {
        centered("all caught up", 94, 2, _gfx->color565(180, 180, 180));
    }
    _gfx->setTextSize(1);
    _gfx->setTextColor(network == "CONNECTED" ? a : _gfx->color565(150, 150, 150));
    _gfx->setCursor(253, 11);
    _gfx->print(network == "CONNECTED" ? "ONLINE" : network);
    footer("tap inbox   hold send   long info");
}

void Display::inbox(const String& sender, const String& text, const String& when,
                    size_t position, size_t count, bool unread, core::Accent accent) {
    clear();
    const uint16_t a = accentColor(accent);
    title("INBOX", a);
    _gfx->setTextSize(1);
    _gfx->setTextColor(_gfx->color565(155, 155, 155));
    _gfx->setCursor(270, 10);
    _gfx->printf("%u/%u", static_cast<unsigned>(position + 1), static_cast<unsigned>(count));
    _gfx->setTextSize(2);
    _gfx->setTextColor(unread ? a : WHITE);
    _gfx->setCursor(10, 42);
    _gfx->print(unread ? "* " : "  ");
    _gfx->print(sender);
    wrapped(text, 10, 72, 300, 2, WHITE, 2);
    _gfx->setTextSize(1);
    _gfx->setTextColor(_gfx->color565(145, 145, 145));
    _gfx->setCursor(10, 136);
    _gfx->print(when);
    footer("tap next   hold mark read   long back");
}

void Display::sendMenu(const char* const* items, size_t count, size_t selected, bool connected, core::Accent accent) {
    clear();
    const uint16_t a = accentColor(accent);
    title("SEND", a);
    const size_t visible = count < 5 ? count : 5;
    const size_t start = selected >= visible ? selected - visible + 1 : 0;
    for (size_t row = 0; row < visible && start + row < count; ++row) {
        const size_t i = start + row;
        _gfx->setTextSize(2);
        _gfx->setTextColor(i == selected ? a : WHITE);
        _gfx->setCursor(16, 38 + static_cast<int>(row) * 21);
        _gfx->print(i == selected ? "> " : "  ");
        _gfx->print(items[i]);
    }
    if (!connected) {
        _gfx->setTextSize(1);
        _gfx->setTextColor(_gfx->color565(190, 190, 190));
        _gfx->setCursor(245, 10);
        _gfx->print("OFFLINE");
    }
    footer("tap next   hold send   long back");
}

void Display::info(size_t page, const String& name, const String& groupCode, const String& groupPassword,
                   const String& wifi, const String& mqtt, const String& accentName,
                   const String& version, core::Accent accent) {
    clear();
    const uint16_t a = accentColor(accent);
    title("INFO", a);
    _gfx->setTextSize(2);
    _gfx->setTextColor(WHITE);
    if (page % 3 == 0) {
        _gfx->setCursor(12, 42); _gfx->print(name);
        _gfx->setTextSize(1);
        _gfx->setCursor(12, 78); _gfx->print("Wi-Fi: " + wifi);
        _gfx->setCursor(12, 95); _gfx->print("MQTT:   " + mqtt);
        _gfx->setCursor(12, 112); _gfx->print("Firmware: " + version);
    } else if (page % 3 == 1) {
        _gfx->setCursor(12, 42); _gfx->print("Room " + groupCode);
        _gfx->setTextSize(1);
        _gfx->setCursor(12, 80); _gfx->print("Password");
        _gfx->setTextSize(3);
        _gfx->setTextColor(a);
        _gfx->setCursor(12, 101); _gfx->print(groupPassword);
    } else {
        _gfx->setCursor(12, 48); _gfx->print("Accent");
        _gfx->setTextSize(3);
        _gfx->setTextColor(a);
        _gfx->setCursor(12, 82); _gfx->print(accentName);
        _gfx->setTextSize(1);
        _gfx->setTextColor(WHITE);
        _gfx->setCursor(12, 126); _gfx->print("Hold to change");
    }
    footer("tap page   hold accent   long back");
}

void Display::notification(const String& titleText, const String& body, core::Accent accent) {
    clear();
    const uint16_t a = accentColor(accent);
    centered(titleText, 40, 2, a);
    wrapped(body, 18, 78, 284, 2, WHITE, 2);
}

void Display::setupMode(const String& apName) {
    clear();
    centered("SETUP MODE", 35, 2, WHITE);
    centered("Connect your phone to", 73, 1, _gfx->color565(175, 175, 175));
    centered(apName, 94, 2, WHITE);
    centered("then open the Wi-Fi sign-in page", 127, 1, _gfx->color565(175, 175, 175));
}

void Display::fatal(const String& message) {
    clear();
    centered("FRIENDBOX ERROR", 35, 2, _gfx->color565(255, 95, 95));
    wrapped(message, 18, 76, 284, 2, WHITE, 3);
}

}  // namespace friendbox::display
