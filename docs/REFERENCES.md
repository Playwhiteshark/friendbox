# Implementation references

Primary or maintainer sources used when choosing APIs and pin mappings:

- LILYGO T-Display-S3 repository: https://github.com/Xinyuan-LilyGO/T-Display-S3
- LILYGO Arduino_GFX example: https://github.com/Xinyuan-LilyGO/T-Display-S3/tree/main/examples/Arduino_GFXDemo
- PlatformIO Espressif32: https://github.com/platformio/platform-espressif32
- PlatformIO T-Display-S3 board docs: https://docs.platformio.org/en/latest/boards/espressif32/lilygo-t-display-s3.html
- Arduino_GFX: https://github.com/moononournation/Arduino_GFX
- WiFiManager: https://github.com/tzapu/WiFiManager
- ArduinoJson: https://arduinojson.org/
- espMqttClient: https://github.com/bertmelis/espMqttClient
- espMqttClient API: https://www.emelis.net/espMqttClient/
- ESP-IDF NVS/Preferences docs: https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
- ESP-IDF OTA docs: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/ota.html
- ESP-IDF HTTP client: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/esp_http_client.html
- ESP x509 certificate bundle: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/esp_crt_bundle.html
- HiveMQ Cloud docs: https://docs.hivemq.com/hivemq-cloud/
- Let's Encrypt certificates: https://letsencrypt.org/certificates/
- GitHub Releases docs: https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases
- GitHub latest-release links: https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases
- PlatformIO GitHub Actions integration: https://docs.platformio.org/en/latest/integration/ci/github-actions.html

Comparable projects were reviewed for scope and interaction patterns, including Pixelix, Meshtastic/MeshCore one-button/headless concepts, and other small ESP32 MQTT displays. No code from those projects is copied into FriendBox.

Comparable implementations reviewed for scope/interaction ideas (architecture only; no copied source):

- Pixelix ESP32 display firmware: https://github.com/BlueAndi/Pixelix
- T-Display-S3 MQTT example project: https://github.com/floresboy/T-Display-S3-MQTT
- Meshtastic single-button Morse PR #9196: https://github.com/meshtastic/firmware/pull/9196
- Meshtastic single-button text/Morse discussion #10299: https://github.com/meshtastic/firmware/issues/10299
