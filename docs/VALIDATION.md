# Validation status

This file separates checks that were actually executed from checks that require the physical T-Display-S3 or a networked PlatformIO runner.

## Executed before packaging

The following checks were run against this repository:

- `FriendBoxCore` compiled with GCC using C++17, `-Wall -Wextra -Werror`, `-fno-exceptions`, and `-fno-rtti`.
- Host unit tests passed for accent parsing/cycling, one-button timing boundaries, group-code/password validation, semantic-version parsing/comparison (including malformed and overflow inputs), and inbox replacement policy.
- OTA manifest-generation tests passed, including version, size, SHA-256, and release-asset URL checks.
- Both GitHub Actions workflow YAML files parsed successfully.
- Repository validation passed for pinned dependencies, board target, OTA partitions, configurable external button, message-store size, bounded MQTT receive queue, and absence of common committed secret-token formats.
- The embedded MQTT CA was extracted and successfully parsed with OpenSSL as `ISRG Root X1`; its public certificate is sourced from Let's Encrypt.
- The source was checked for accidental `setInsecure()` TLS use and exception-dependent embedded code.

## API/documentation verification

Implementation calls were checked against the maintained/vendor interfaces used by the repository:

- LILYGO's T-Display-S3 Arduino_GFX example for display/peripheral pins.
- PlatformIO's `lilygo-t-display-s3` board target and Espressif32 platform.
- WiFiManager 2.0.17 constructors and captive-portal methods.
- espMqttClient 1.7.3 TLS, QoS, persistent-session, callback, constructor, and client-ID-size APIs.
- ESP-IDF HTTP client, x509 certificate bundle, OTA partition/write, SHA-256, and post-boot validation APIs compatible with the Arduino framework bundled by the pinned PlatformIO platform. The pinned PlatformIO platform selects Arduino-ESP32 2.0.17; that ESP32-S3 SDK configuration enables both the full certificate bundle and bootloader app rollback.
- HiveMQ Cloud's username/password authentication and topic-filter permission model.
- GitHub's Release/tag model and stable latest-release asset links.

## Not claimed as physically validated yet

These checks cannot be truthfully completed without the actual board and/or a live broker/repository:

1. Full PlatformIO firmware compile in the packaging environment. PlatformIO Core is not installed here and this shell cannot reach package registries. The included GitHub `build` workflow performs that exact full build on every push/PR.
2. LCD orientation/backlight appearance on the user's exact T-Display-S3 board revision.
3. GPIO1 external-button electrical smoke test.
4. Two-device HiveMQ Cloud exchange and offline queued delivery.
5. GitHub OTA happy path on hardware.
6. Automatic bootloader rollback. The source SDK configuration enables it and the app has A/B partitions plus the ESP-IDF validation hook, but the destructive hardware test must still prove the exact prebuilt bootloader that PlatformIO flashes.

Use `docs/HARDWARE_VALIDATION.md` for the intentionally short physical test sequence. Do not describe rollback as validated until its destructive test passes.

## Physical two-device validation — 2026-08-15

Validated on two LILYGO T-Display-S3 devices:

- both devices boot successfully
- captive setup portal works
- unique setup AP names work
- Wi-Fi configuration works
- both devices connect to HiveMQ Cloud over MQTT
- one device can create a FriendBox room
- second device can join using the same room credentials
- messages successfully send and receive between both devices
- persistent local configuration survives reboot

Result: PASS