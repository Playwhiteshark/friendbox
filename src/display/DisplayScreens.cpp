#include "Display.h"

#include "ProductInfo.h"

namespace friendbox::display {
namespace {
constexpr Area kMenuBody{16, 38, 294, 105};
constexpr Area kPageIndicator{270, 10, 40, 8};
constexpr size_t kInfoPageCount = 4;
}

void Display::boot(const String& line) {
    clear();
    centered({0, 55, 320, 24}, product::kName, 3, TFT_WHITE);
    centered({0, 96, 320, 8}, line, 1, _gfx.color565(155, 155, 155));
}

void Display::idle(const String& timeText, size_t unread, const String& network,
                   uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title(product::kDisplayTitle, a);
    drawClock({0, 48, 320, 32}, timeText, 4, TFT_WHITE);
    if (unread > 0) {
        centered({0, timeText.isEmpty() ? 67 : 91, 320, 16},
                 String(unread) + (unread == 1 ? " NEW MESSAGE" : " NEW MESSAGES"), 2, a);
    } else {
        centered({0, timeText.isEmpty() ? 70 : 94, 320, 16}, "all caught up", 2,
                 _gfx.color565(180, 180, 180));
    }
    drawText({253, 11, 57, 8}, network == "CONNECTED" ? "ONLINE" : network, 1,
             network == "CONNECTED" ? a : _gfx.color565(150, 150, 150));
    footer("tap inbox   hold send   long more");
}

void Display::inbox(const String& sender, const String& text, const String& when,
                    size_t position, size_t count, bool unread, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title("INBOX", a);
    drawPageIndicator(kPageIndicator, position, count, _gfx.color565(155, 155, 155));
    drawText({10, 42, 300, 16}, String(unread ? "* " : "  ") + sender, 2,
             unread ? a : TFT_WHITE);
    wrapped({10, 72, 300, 40}, text, 2, TFT_WHITE, 2);
    drawText({10, 136, 300, 8}, when, 1, _gfx.color565(145, 145, 145));
    footer("tap next   hold mark read   long back");
}

void Display::sendMenu(const std::string* items, size_t count, size_t selected,
                       bool connected, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title("SEND", a);
    drawMenuList(kMenuBody, items, count, selected, a);
    if (!connected) {
        drawText({245, 10, 65, 8}, "OFFLINE", 1, _gfx.color565(190, 190, 190));
    }
    footer("tap next   hold send   long back");
}

void Display::selectionMenu(const String& heading, const std::string* items, size_t count,
                            size_t selected, const String& help, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title(heading, a);
    drawMenuList(kMenuBody, items, count, selected, a);
    footer(help);
}

void Display::composer(const String& heading, const std::string& text, const std::string& code,
                       char preview, bool invalid, bool full, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    title(heading, a);
    String body(text.c_str());
    if (body.length() > 69) body = "..." + body.substring(body.length() - 66);
    wrapped({12, 39, 296, 60}, body.isEmpty() ? String("start typing...") : body, 2,
            body.isEmpty() ? _gfx.color565(145, 145, 145) : TFT_WHITE, 3);

    drawText({12, 108, 200, 24}, code.empty() ? String("_") : String(code.c_str()), 3, a);
    const uint16_t previewColor = invalid || full ? _gfx.color565(255, 95, 95) : TFT_WHITE;
    if (full) drawText({226, 112, 82, 16}, "FULL", 2, previewColor);
    else if (invalid) drawText({226, 112, 82, 16}, "INVALID", 2, previewColor);
    else if (preview != '\0') drawText({226, 112, 82, 16}, String(preview), 2, previewColor);
    footer("short .   longer -   very long menu");
}

void Display::info(size_t page, const String& name, const String& groupCode,
                   const String& groupPassword, const String& wifi, const String& mqtt,
                   const String& ota, const String& otaDetail, const String& accentName,
                   const String& version, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    const size_t infoPage = page % kInfoPageCount;
    title("INFO", a);
    drawPageIndicator(kPageIndicator, infoPage, kInfoPageCount,
                      _gfx.color565(155, 155, 155));
    if (infoPage == 0) {
        drawText({12, 42, 296, 16}, name, 2, TFT_WHITE);
        const DetailRow rows[] = {
            {"Wi-Fi: ", wifi},
            {"MQTT:   ", mqtt},
            {"Firmware: ", version},
        };
        drawDetailList({12, 78, 296, 51}, rows, 3, 1, TFT_WHITE);
    } else if (infoPage == 1) {
        drawText({12, 42, 296, 16}, "Room " + groupCode, 2, TFT_WHITE);
        drawText({12, 80, 296, 8}, "Password", 1, TFT_WHITE);
        drawText({12, 101, 296, 24}, groupPassword, 3, a);
    } else if (infoPage == 2) {
        drawText({12, 48, 296, 16}, "Accent", 2, TFT_WHITE);
        drawText({12, 82, 296, 24}, accentName, 3, a);
        drawText({12, 126, 296, 8}, "Change from phone setup", 1, TFT_WHITE);
    } else {
        drawText({12, 48, 296, 16}, "Software update", 2, TFT_WHITE);
        drawText({12, 82, 296, 16}, ota, 2, a);
        if (otaDetail.isEmpty()) {
            drawText({12, 114, 296, 8}, "Checks after startup, then every 12h", 1, TFT_WHITE);
        } else {
            wrapped({12, 110, 296, 22}, otaDetail, 1, TFT_WHITE, 2);
        }
    }
    footer("tap page   long back");
}

void Display::notification(const String& titleText, const String& body, uint32_t accentRgb) {
    clear();
    const uint16_t a = accentColor(accentRgb);
    centered({0, 40, 320, 16}, titleText, 2, a);
    wrapped({18, 78, 284, 40}, body, 2, TFT_WHITE, 2);
}

void Display::setupMode(const String& apName) {
    clear();
    centered({0, 35, 320, 16}, "SETUP MODE", 2, TFT_WHITE);
    centered({0, 73, 320, 8}, "Connect your phone to", 1, _gfx.color565(175, 175, 175));
    centered({0, 94, 320, 16}, apName, 2, TFT_WHITE);
    centered({0, 127, 320, 8}, "then open the Wi-Fi sign-in page", 1,
             _gfx.color565(175, 175, 175));
}

void Display::fatal(const String& message) {
    clear();
    centered({0, 35, 320, 16}, String(product::kDisplayTitle) + " ERROR", 2,
             _gfx.color565(255, 95, 95));
    wrapped({18, 76, 284, 60}, message, 2, TFT_WHITE, 3);
}

}  // namespace friendbox::display
