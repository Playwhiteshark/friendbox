# Implementation references

Primary vendor or maintainer sources used for APIs, pins, build configuration, and protocol behavior:

## Board, framework, and display

- LILYGO T-Display-S3 repository: https://github.com/Xinyuan-LilyGO/T-Display-S3
- TFT_eSPI: https://github.com/Bodmer/TFT_eSPI
- TFT_eSPI T-Display-S3 setup (`Setup206_LilyGo_T_Display_S3.h`): https://github.com/Bodmer/TFT_eSPI/blob/master/User_Setups/Setup206_LilyGo_T_Display_S3.h
- PlatformIO Espressif32: https://github.com/platformio/platform-espressif32
- PlatformIO Espressif32 6.5.0 release: https://github.com/platformio/platform-espressif32/releases/tag/v6.5.0
- PlatformIO T-Display-S3 board documentation: https://docs.platformio.org/en/latest/boards/espressif32/lilygo-t-display-s3.html
- Arduino-ESP32 2.0.14 release: https://github.com/espressif/arduino-esp32/releases/tag/2.0.14

The known-good FriendBox build deliberately pins PlatformIO Espressif32 6.5.0, Arduino-ESP32 2.0.14, TFT_eSPI 2.5.43, and DIO flash mode. Older documentation referred to Arduino_GFX and PlatformIO 7.0.1; those are not the current implementation.

## Setup, storage, JSON, and MQTT

- WiFiManager: https://github.com/tzapu/WiFiManager
- ArduinoJson: https://arduinojson.org/
- espMqttClient: https://github.com/bertmelis/espMqttClient
- espMqttClient API: https://www.emelis.net/espMqttClient/
- Arduino-ESP32 Preferences/NVS tutorial: https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
- HiveMQ Cloud documentation: https://docs.hivemq.com/hivemq-cloud/
- Let's Encrypt certificates: https://letsencrypt.org/certificates/

## OTA and release infrastructure

- ESP-IDF OTA documentation: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/ota.html
- ESP-IDF HTTP client: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/esp_http_client.html
- ESP x509 certificate bundle: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/esp_crt_bundle.html
- GitHub Releases: https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases
- GitHub latest-release asset links: https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases
- PlatformIO with GitHub Actions: https://docs.platformio.org/en/latest/integration/ci/github-actions.html

## Comparable projects

Comparable projects were reviewed for scope and interaction patterns only; no source was copied into FriendBox:

- Pixelix ESP32 display firmware: https://github.com/BlueAndi/Pixelix
- T-Display-S3 MQTT example: https://github.com/floresboy/T-Display-S3-MQTT
- Meshtastic single-button Morse PR #9196: https://github.com/meshtastic/firmware/pull/9196
- Meshtastic single-button text/Morse discussion #10299: https://github.com/meshtastic/firmware/issues/10299
