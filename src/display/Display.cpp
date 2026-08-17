#include "Display.h"

namespace friendbox::display {
namespace {
constexpr int16_t kWidth = 320;

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8U) << 8U) |
                                 ((g & 0xFCU) << 3U) |
                                 (b >> 3U));
}

constexpr uint16_t kAccentColors[] = {
    rgb565(0, 220, 230),
    rgb565(70, 135, 255),
    rgb565(70, 220, 120),
    rgb565(255, 155, 55),
    rgb565(255, 100, 175),
    rgb565(175, 105, 255),
};

static_assert(sizeof(kAccentColors) / sizeof(kAccentColors[0]) ==
              static_cast<size_t>(core::Accent::Count));
}

bool Display::begin() {
    // TFT_eSPI Setup206_LilyGo_T_Display_S3.h contains the board's official
    // ST7789 170x320 8-bit parallel pin mapping. Board::begin() powers the
    // display rail and backlight before this function is called.
    _gfx.init();
    _gfx.setRotation(1);
    _gfx.setTextWrap(false, false);
    clear();
    return true;
}

void Display::clear() {
    _gfx.fillScreen(TFT_BLACK);
}

uint16_t Display::accentColor(core::Accent accent) const {
    const size_t index = static_cast<size_t>(accent);
    return index < static_cast<size_t>(core::Accent::Count) ? kAccentColors[index] : TFT_WHITE;
}

void Display::centered(const String& text, int16_t y, uint8_t size, uint16_t color) {
    _gfx.setTextSize(size);
    _gfx.setTextColor(color);
    // Font 1 is the built-in 6x8 GLCD font used by setTextSize().
    const int16_t approximateWidth = static_cast<int16_t>(text.length() * 6U * size);
    const int16_t centeredX = (kWidth - approximateWidth) / 2;
    const int16_t x = centeredX < 4 ? 4 : centeredX;
    _gfx.setCursor(x, y);
    _gfx.print(text);
}

void Display::title(const String& text, uint16_t accent) {
    _gfx.fillRect(0, 0, kWidth, 29, TFT_BLACK);
    _gfx.setTextSize(2);
    _gfx.setTextColor(accent);
    _gfx.setCursor(10, 8);
    _gfx.print(text);
    _gfx.drawFastHLine(10, 28, 300, accent);
}

void Display::footer(const String& text) {
    _gfx.fillRect(0, 149, kWidth, 21, TFT_BLACK);
    _gfx.setTextSize(1);
    _gfx.setTextColor(_gfx.color565(150, 150, 150));
    _gfx.setCursor(10, 156);
    _gfx.print(text);
}

void Display::wrapped(const String& text, int16_t x, int16_t y, int16_t maxWidth,
                      uint8_t size, uint16_t color, uint8_t maxLines) {
    _gfx.setTextSize(size);
    _gfx.setTextColor(color);
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
        _gfx.setCursor(x, y + line * (8 * size + 3));
        _gfx.print(part);
        remaining = remaining.substring(min(remaining.length(), take + (take < remaining.length() && remaining[take] == ' ' ? 1 : 0)));
    }
}

}  // namespace friendbox::display
