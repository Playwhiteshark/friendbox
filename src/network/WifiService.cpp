#include "WifiService.h"
#include <WiFi.h>
#include "BuildConfig.h"

namespace friendbox::network {

void WifiService::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin();
    }
    _lastReconnect = millis();
}

void WifiService::update() {
    if (connected()) return;
    const uint32_t now = millis();
    if (now - _lastReconnect >= build::kWifiReconnectMs) {
        _lastReconnect = now;
        WiFi.reconnect();
    }
}

bool WifiService::connected() const { return WiFi.status() == WL_CONNECTED; }

WifiStatus WifiService::status() const {
    if (connected()) return WifiStatus::Connected;
    const wl_status_t s = WiFi.status();
    return (s == WL_IDLE_STATUS || s == WL_DISCONNECTED) ? WifiStatus::Connecting : WifiStatus::Offline;
}

String WifiService::label() const {
    switch (status()) {
        case WifiStatus::Connected: return "CONNECTED";
        case WifiStatus::Connecting: return "CONNECTING";
        default: return "OFFLINE";
    }
}

}  // namespace friendbox::network
