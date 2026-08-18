#include "Display.h"

#include "ProductInfo.h"

namespace friendbox::display {

void Display::boot(const String& line) {
    clear();
    centered(product::kName, 55, 3, TFT_WHITE);
    centered(line, 96, 1, _gfx.color565(155, 155, 155));
}

void Display::idle(const String& timeText, size_t unread, const String& network,
                   uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title(product::kDisplayTitle, a);
    if (!timeText.isEmpty()) centered(timeText, 48, 4, TFT_WHITE);
    if (unread > 0) {
        centered(String(unread) + (unread == 1 ? " NEW MESSAGE" : " NEW MESSAGES"),
                 timeText.isEmpty() ? 67 : 91, 2, a);
    } else {
        centered("all caught up", timeText.isEmpty() ? 70 : 94, 2,
                 _gfx.color565(180, 180, 180));
    }
    _gfx.setTextSize(1);
    _gfx.setTextColor(network == "CONNECTED" ? a : _gfx.color565(150, 150, 150));
    _gfx.setCursor(253, 11);
    _gfx.print(network == "CONNECTED" ? "ONLINE" : network);
    footer("tap inbox   hold send   long more");
}

void Display::inbox(const String& sender, const String& text, const String& when,
                    size_t position, size_t count, bool unread, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title("INBOX", a, position, count);
    _gfx.setTextSize(2);
    _gfx.setTextColor(unread ? a : TFT_WHITE);
    _gfx.setCursor(10, 42);
    _gfx.print(unread ? "* " : "  ");
    _gfx.print(sender);
    wrapped(text, 10, 72, 300, 2, TFT_WHITE, 2);
    _gfx.setTextSize(1);
    _gfx.setTextColor(_gfx.color565(145, 145, 145));
    _gfx.setCursor(10, 136);
    _gfx.print(when);
    footer("tap next   hold mark read   long back");
}

void Display::sendMenu(const std::string* items, size_t count, size_t selected,
                       bool connected, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title("SEND", a);
    const size_t visible = count < 5 ? count : 5;
    const size_t start = selected >= visible ? selected - visible + 1 : 0;
    for (size_t row = 0; row < visible && start + row < count; ++row) {
        const size_t i = start + row;
        _gfx.setTextSize(2);
        _gfx.setTextColor(i == selected ? a : TFT_WHITE);
        _gfx.setCursor(16, 38 + static_cast<int>(row) * 21);
        _gfx.print(i == selected ? "> " : "  ");
        String label(items[i].c_str());
        if (label.length() > 22) label = label.substring(0, 19) + "...";
        _gfx.print(label);
    }
    moreBelow(start + visible < count);
    if (!connected) {
        _gfx.setTextSize(1);
        _gfx.setTextColor(_gfx.color565(190, 190, 190));
        _gfx.setCursor(245, 10);
        _gfx.print("OFFLINE");
    }
    footer("tap next   hold send   long back");
}

void Display::selectionMenu(const String& heading, const std::string* items, size_t count,
                            size_t selected, const String& help, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title(heading, a);
    const size_t visible = count < 5 ? count : 5;
    const size_t start = selected >= visible ? selected - visible + 1 : 0;
    for (size_t row = 0; row < visible && start + row < count; ++row) {
        const size_t i = start + row;
        _gfx.setTextSize(2);
        _gfx.setTextColor(i == selected ? a : TFT_WHITE);
        _gfx.setCursor(16, 38 + static_cast<int>(row) * 21);
        String label(items[i].c_str());
        if (label.length() > 22) label = label.substring(0, 19) + "...";
        _gfx.print(i == selected ? "> " : "  ");
        _gfx.print(label);
    }
    moreBelow(start + visible < count);
    footer(help);
}

void Display::composer(const String& heading, const std::string& text, const std::string& code,
                       char preview, bool invalid, bool full, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title(heading, a);
    String body(text.c_str());
    if (body.length() > 69) body = "..." + body.substring(body.length() - 66);
    wrapped(body.isEmpty() ? String("start typing...") : body, 12, 39, 296, 2,
            body.isEmpty() ? _gfx.color565(145, 145, 145) : TFT_WHITE, 3);

    _gfx.setTextSize(3);
    _gfx.setTextColor(a);
    _gfx.setCursor(12, 108);
    _gfx.print(code.empty() ? "_" : code.c_str());
    _gfx.setTextSize(2);
    _gfx.setTextColor(invalid || full ? _gfx.color565(255, 95, 95) : TFT_WHITE);
    _gfx.setCursor(226, 112);
    if (full) _gfx.print("FULL");
    else if (invalid) _gfx.print("INVALID");
    else if (preview != '\0') _gfx.print(preview);
    footer("short .   longer -   very long menu");
}

void Display::info(size_t page, const String& name, const String& groupCode,
                   const String& groupPassword, const String& wifi, const String& mqtt,
                   const String& ota, const String& otaDetail, const String& accentName,
                   const String& version, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title("INFO", a, page % 4, 4);
    _gfx.setTextSize(2);
    _gfx.setTextColor(TFT_WHITE);
    if (page % 4 == 0) {
        _gfx.setCursor(12, 42); _gfx.print(name);
        _gfx.setTextSize(1);
        _gfx.setCursor(12, 78); _gfx.print("Wi-Fi: " + wifi);
        _gfx.setCursor(12, 95); _gfx.print("MQTT:   " + mqtt);
        _gfx.setCursor(12, 112); _gfx.print("Firmware: " + version);
    } else if (page % 4 == 1) {
        _gfx.setCursor(12, 42); _gfx.print("Room " + groupCode);
        _gfx.setTextSize(1);
        _gfx.setCursor(12, 80); _gfx.print("Password");
        _gfx.setTextSize(3);
        _gfx.setTextColor(a);
        _gfx.setCursor(12, 101); _gfx.print(groupPassword);
    } else if (page % 4 == 2) {
        _gfx.setCursor(12, 48); _gfx.print("Accent");
        _gfx.setTextSize(3);
        _gfx.setTextColor(a);
        _gfx.setCursor(12, 82); _gfx.print(accentName);
        _gfx.setTextSize(1);
        _gfx.setTextColor(TFT_WHITE);
        _gfx.setCursor(12, 126); _gfx.print("Change from phone setup");
    } else {
        _gfx.setCursor(12, 48); _gfx.print("Software update");
        _gfx.setTextSize(2);
        _gfx.setTextColor(a);
        _gfx.setCursor(12, 82); _gfx.print(ota);
        _gfx.setTextSize(1);
        _gfx.setTextColor(TFT_WHITE);
        if (otaDetail.isEmpty()) {
            _gfx.setCursor(12, 114); _gfx.print("Checks after startup, then every 12h");
        } else {
            wrapped(otaDetail, 12, 110, 296, 1, TFT_WHITE, 2);
        }
    }
    footer("tap page   long back");
}

void Display::notification(const String& titleText, const String& body, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    centered(titleText, 40, 2, a);
    wrapped(body, 18, 78, 284, 2, TFT_WHITE, 2);
}

void Display::setupMode(const String& apName) {
    clear();
    centered("SETUP MODE", 35, 2, TFT_WHITE);
    centered("Connect your phone to", 73, 1, _gfx.color565(175, 175, 175));
    centered(apName, 94, 2, TFT_WHITE);
    centered("then open the Wi-Fi sign-in page", 127, 1, _gfx.color565(175, 175, 175));
}

void Display::fatal(const String& message) {
    clear();
    centered(String(product::kDisplayTitle) + " ERROR", 35, 2, _gfx.color565(255, 95, 95));
    wrapped(message, 18, 76, 284, 2, TFT_WHITE, 3);
}

}  // namespace friendbox::display
