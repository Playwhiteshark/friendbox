#pragma once

#include <stdint.h>

namespace friendbox::service {

// One public source of truth for the normal MQTT-over-TLS port. The private
// bootstrap file only overrides it if a different broker port is genuinely
// required.
inline constexpr uint16_t kDefaultMqttTlsPort = 8883;

}  // namespace friendbox::service

// LocalServiceConfig.h is intentionally private and gitignored. It may be
// present on the developer machine used for the first USB flash, but it is
// absent from public GitHub/OTA builds.
#if __has_include("LocalServiceConfig.h")
#include "LocalServiceConfig.h"
#endif

// Keep public builds compilable when no private provisioning file exists.
#ifndef FRIEND_BOX_LOCAL_MQTT_HOST
#define FRIEND_BOX_LOCAL_MQTT_HOST ""
#endif
#ifndef FRIEND_BOX_LOCAL_MQTT_PORT
#define FRIEND_BOX_LOCAL_MQTT_PORT friendbox::service::kDefaultMqttTlsPort
#endif
#ifndef FRIEND_BOX_LOCAL_MQTT_USERNAME
#define FRIEND_BOX_LOCAL_MQTT_USERNAME ""
#endif
#ifndef FRIEND_BOX_LOCAL_MQTT_PASSWORD
#define FRIEND_BOX_LOCAL_MQTT_PASSWORD ""
#endif

namespace friendbox::service {

struct BootstrapDefaults {
    const char* host;
    uint16_t port;
    const char* username;
    const char* password;

    bool complete() const {
        return host != nullptr && host[0] != '\0' &&
               port > 0 &&
               username != nullptr && username[0] != '\0' &&
               password != nullptr && password[0] != '\0';
    }
};

static_assert(FRIEND_BOX_LOCAL_MQTT_PORT > 0 && FRIEND_BOX_LOCAL_MQTT_PORT <= 65535,
              "FRIEND_BOX_LOCAL_MQTT_PORT must be between 1 and 65535");

inline constexpr BootstrapDefaults kBootstrapDefaults{
    FRIEND_BOX_LOCAL_MQTT_HOST,
    static_cast<uint16_t>(FRIEND_BOX_LOCAL_MQTT_PORT),
    FRIEND_BOX_LOCAL_MQTT_USERNAME,
    FRIEND_BOX_LOCAL_MQTT_PASSWORD,
};

inline bool bootstrapDefaultsAvailable() {
    return kBootstrapDefaults.complete();
}

inline constexpr bool shouldSeedBootstrap(bool serviceInitialized,
                                          bool hasMeaningfulStoredSettings,
                                          bool defaultsAvailable) {
    return !serviceInitialized && !hasMeaningfulStoredSettings && defaultsAvailable;
}

}  // namespace friendbox::service
