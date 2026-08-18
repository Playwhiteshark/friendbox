#include "Display.h"

namespace friendbox::display {
namespace {
constexpr int16_t kWidth = 320;

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8U) << 8U) |
                                 ((g & 0xFCU) << 3U) |
                                 (b >> 3U));
}

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

uint16_t Display::accentColor(uint32_t rgb) const {
    return rgb565(static_cast<uint8_t>((rgb >> 16U) & 0xFFU),
                  static_cast<uint8_t>((rgb >> 8U) & 0xFFU),
                  static_cast<uint8_t>(rgb & 0xFFU));
}

void Display::drawText(const Area& area, const String& text, uint8_t size, uint16_t color) {
    _gfx.setTextSize(size);
    _gfx.setTextColor(color);
    _gfx.setCursor(area.x, area.y);
    _gfx.print(text);
}

void Display::centered(const Area& area, const String& text, uint8_t size, uint16_t color) {
    _gfx.setTextSize(size);
    _gfx.setTextColor(color);
    // Font 1 is the built-in 6x8 GLCD font used by setTextSize().
    const int16_t approximateWidth = static_cast<int16_t>(text.length() * 6U * size);
    const int16_t centeredX = area.x + (area.width - approximateWidth) / 2;
    const int16_t minimumX = area.x < 4 ? 4 : area.x;
    const int16_t x = centeredX < minimumX ? minimumX : centeredX;
    _gfx.setCursor(x, area.y);
    _gfx.print(text);
}

void Display::title(const String& text, uint16_t accent) {
    _gfx.fillRect(0, 0, kWidth, 29, TFT_BLACK);
    drawText({10, 8, 300, 16}, text, 2, accent);
    _gfx.drawFastHLine(10, 28, 300, accent);
}

void Display::footer(const String& text) {
    _gfx.fillRect(0, 149, kWidth, 21, TFT_BLACK);
    drawText({10, 156, 300, 8}, text, 1, _gfx.color565(150, 150, 150));
}

void Display::wrapped(const Area& area, const String& text, uint8_t size,
                      uint16_t color, uint8_t maxLines) {
    _gfx.setTextSize(size);
    _gfx.setTextColor(color);
    const int charWidth = 6 * size;
    const size_t calculatedChars = static_cast<size_t>(area.width / charWidth);
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
        _gfx.setCursor(area.x, area.y + line * (8 * size + 3));
        _gfx.print(part);
        remaining = remaining.substring(min(remaining.length(), take + (take < remaining.length() && remaining[take] == ' ' ? 1 : 0)));
    }
}

void Display::drawClock(const Area& area, const String& timeText, uint8_t size, uint16_t color) {
    if (!timeText.isEmpty()) centered(area, timeText, size, color);
}

void Display::drawPageIndicator(const Area& area, size_t position, size_t count, uint16_t color) {
    if (count == 0) return;
    const size_t shownPosition = position < count ? position : count - 1;
    drawText(area, String(shownPosition + 1) + "/" + String(count), 1, color);
}

void Display::drawMenuList(const Area& area, const std::string* items, size_t count,
                           size_t selected, uint16_t accent) {
    constexpr int16_t kRowHeight = 21;
    constexpr uint8_t kTextSize = 2;
    if (!items || count == 0 || area.height < kRowHeight) return;

    const size_t rowCapacity = static_cast<size_t>(area.height / kRowHeight);
    const size_t visible = count < rowCapacity ? count : rowCapacity;
    const size_t start = selected >= visible ? selected - visible + 1 : 0;
    const size_t charsAcross = area.width > 0
                                   ? static_cast<size_t>(area.width / (6 * kTextSize))
                                   : 0;
    const size_t maxLabelChars = charsAcross > 2 ? charsAcross - 2 : 1;

    for (size_t row = 0; row < visible && start + row < count; ++row) {
        const size_t i = start + row;
        _gfx.setTextSize(kTextSize);
        _gfx.setTextColor(i == selected ? accent : TFT_WHITE);
        _gfx.setCursor(area.x, area.y + static_cast<int16_t>(row) * kRowHeight);
        String label(items[i].c_str());
        if (label.length() > maxLabelChars) {
            const size_t keep = maxLabelChars > 3 ? maxLabelChars - 3 : maxLabelChars;
            label = label.substring(0, keep);
            if (maxLabelChars > 3) label += "...";
        }
        _gfx.print(i == selected ? "> " : "  ");
        _gfx.print(label);
    }

    if (start + visible < count) {
        constexpr int16_t kHintWidth = 36;
        const int16_t hintX = area.x + area.width - kHintWidth;
        const int16_t hintY = area.y + static_cast<int16_t>(visible) * kRowHeight - 3;
        drawText({hintX, hintY, kHintWidth, 8}, "more v", 1,
                 _gfx.color565(150, 150, 150));
    }
}

void Display::drawDetailList(const Area& area, const DetailRow* rows, size_t count,
                             uint8_t size, uint16_t color, int16_t rowHeight) {
    if (!rows || count == 0 || rowHeight <= 0) return;
    const size_t capacity = area.height > 0
                                ? static_cast<size_t>(area.height / rowHeight)
                                : count;
    const size_t visible = count < capacity ? count : capacity;
    for (size_t i = 0; i < visible; ++i) {
        drawText({area.x, area.y + static_cast<int16_t>(i) * rowHeight,
                  area.width, rowHeight},
                 rows[i].label + rows[i].value, size, color);
    }
}

}  // namespace friendbox::display
