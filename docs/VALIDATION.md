# Validation status

This file records what has actually passed. It deliberately separates automated checks, physical results, and remaining hardware work.

## Automated checks

The `build` workflow runs on every push and pull request. The `main` run for firmware commit `1c66ac7` passed on 2026-08-15, and the documentation update was also gated by the same successful workflow. It includes:

- host compilation/tests for `FriendBoxCore` behavior;
- host tests for local service bootstrap precedence;
- OTA manifest-generation tests;
- repository checks for the pinned board/toolchain/libraries;
- partition alignment, size, and equal A/B application-slot checks;
- configured GitHub repository-slug verification in CI;
- MQTT CA extraction and OpenSSL certificate parsing;
- checks against disabled TLS verification and common committed token formats; and
- a complete `pio run -e friendbox` firmware build.

The host core coverage includes accent parsing/RGB values, button timing boundaries, room credential validation, strict numeric semantic-version comparison (including malformed/overflow input), inbox replacement policy, disabled preset mapping, Morse decoding/composition, and composer/navigation intents.

Run the same local entry point with:

```bash
./scripts/check.sh
```

If PlatformIO is installed, it includes the complete firmware build; otherwise the script explicitly reports that only host/repository checks ran.

## Physical validation completed — 2026-08-15

Validated on two LILYGO T-Display-S3 devices:

- firmware built, flashed, and booted;
- the ST7789 display initialized correctly with `TFT_eSPI 2.5.43`;
- DIO flash mode booted reliably;
- the captive setup portal worked;
- each unconfigured device exposed a unique setup AP using its full device ID;
- settings persisted across reboot;
- accent selection/configuration worked;
- both devices authenticated to HiveMQ Cloud over TLS;
- one device created a room and the other joined with the same credentials; and
- messages were successfully exchanged between the two devices.

Result: **PASS for the basic two-device messaging system.**

On the current Windows development machine, normal esptool stub uploads repeatedly disconnected at a similar point. Entering ESP32-S3 ROM download mode and flashing with `esptool --no-stub` was reliable. This is tracked as a development-machine upload issue, not a FriendBox runtime failure.

## OTA validation attempt — 2026-08-18

The `v0.1.0` release workflow passed and published a 1,095,584-byte firmware image plus a matching SHA-256 manifest. A configured device running `0.1.0-dev` had working Wi-Fi and network time but remained on the development version after reboot and the initial update window. A diagnostic USB build then reported `MANIFEST REQUEST`, ESP error `28674`, and `arduino_esp_crt_bundle_attach(): Failed to attach bundle`. Root cause: Arduino-ESP32 2.x exposes the certificate-bundle callback but does not provide bundle data until the application calls its setter. The follow-up build embedded and initialized a reviewed public-root bundle, reached GitHub, then reported `MANIFEST REQUEST HTTP 302 ERR -1`. ESP-IDF 4.4.6's 512-byte default transmit buffer could not hold GitHub's long signed redirect URL; the next patch uses a 2 KiB transmit buffer for both OTA requests. Physical retest remains pending.

## Implemented but not fully validated

These paths exist in code but still need explicit end-to-end proof:

1. Phone-configurable presets/display settings on iPhone and physical Morse timing/on-device preset editing.
2. Screen timeout behavior, especially the guarantee that incoming messages never wake the display.
3. A complete GitHub OTA update from one tagged release to a newer tagged release.
4. Automatic rollback after installing a deliberately unhealthy test image. A/B partitions, OTA metadata, and the app validation hook exist; the exact flashed bootloader behavior remains the deciding test.
5. Extended offline/reconnect testing, including broker-queued QoS 1 delivery after a longer disconnection and duplicate-delivery behavior.
6. The final external GPIO1 momentary button mounted in the finished enclosure.

Release `v0.1.0` exists as the baseline test image. Do not describe OTA or rollback as physically validated until the remaining hardware checks pass.

Use [Physical validation checklist](HARDWARE_VALIDATION.md) for the remaining repeatable tests.
