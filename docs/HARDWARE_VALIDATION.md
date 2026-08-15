# Physical validation checklist

These are deliberately a few high-value tests, not a large hardware test suite.

## 1. Boot/display/button smoke test

- Wire one normally-open momentary button between **GPIO1 and GND**.
- Flash the firmware over USB-C.
- Confirm the screen starts and reaches setup/idle.
- Confirm tap opens Inbox, hold opens Send, and long hold opens Info.
- In Info, hold several times and confirm the accent color changes and survives a reboot.

## 2. Two-box messaging test

- Create one room on box A.
- Join that room on box B.
- Send each canned message both directions.
- Confirm sender messages do not echo into its own inbox.
- Confirm received messages survive reboot.
- Mark a message read and confirm read state survives reboot.

## 3. Temporary-offline test

- With both boxes working, disconnect box B from Wi-Fi without changing its saved configuration.
- Send a QoS 1 message from A.
- Restore B's connection and confirm the broker's persistent session delivers the queued message.
- This test depends on the broker retaining the MQTT 3.1.1 persistent session.

## 4. OTA happy-path test

- Flash an older version by USB.
- Publish a newer GitHub release tag.
- Confirm the device discovers the latest manifest, downloads the inactive partition, reboots, and Info shows the new version.

## 5. Rollback test — required before relying on rollback

Because rollback is a bootloader feature, this is the one destructive test that must be proven on the exact PlatformIO/Arduino package:

1. Keep known-good firmware in the current slot.
2. OTA a deliberately test-only firmware that crashes/reboots before the application can mark itself valid.
3. Confirm the bootloader returns to the old slot.
4. If it does not, automatic rollback is **not validated** for that toolchain. Keep SHA-verified A/B OTA, but either use a rollback-enabled bootloader/framework build or continue doing manual recovery over USB.

Do not ship the deliberately broken test image as a normal release.
