#pragma once

// Copy this file to include/LocalServiceConfig.h and fill in the MQTT
// credentials for boxes that you personally provision over USB.
//
// include/LocalServiceConfig.h is gitignored and MUST NOT be committed.
// These values are copied into NVS only when the device has never had MQTT
// service settings before. Normal runtime and OTA updates use the NVS copy.

#define FRIEND_BOX_LOCAL_MQTT_HOST "your-cluster.s1.eu.hivemq.cloud"
// Optional: omit this line to use FriendBox's standard MQTT TLS port (8883).
#define FRIEND_BOX_LOCAL_MQTT_PORT 8883
#define FRIEND_BOX_LOCAL_MQTT_USERNAME "friendbox"
#define FRIEND_BOX_LOCAL_MQTT_PASSWORD "replace-me"
