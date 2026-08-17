# Physical validation checklist

This is a short set of high-value physical tests. Current status is summarized in [Validation status](VALIDATION.md).

## 1. Boot, display, and final button

Current status: display/boot and basic UI behavior passed; the final external GPIO1 button in its enclosure is still pending.

- Wire one normally-open momentary button between **GPIO1 and GND**.
- Flash the firmware over USB-C using DIO flash mode.
- Confirm the screen reaches setup or Idle without a reset loop.
- Confirm tap opens Inbox, hold opens Send, and long hold opens More.
- Confirm More opens Edit Presets and the read-only Info pages.
- Change the accent, brightness, screen timeout, and clock visibility from phone setup; confirm they survive reboot.
- Confirm a five-second startup hold opens setup mode.

If normal uploads disconnect on the current Windows development machine, enter ROM download mode (hold BOOT, tap RST, release BOOT) and use the equivalent esptool command with `--no-stub`.

## 2. Phone settings and Morse

Current status: implemented and host-tested; physical timing and captive-portal layout need validation.

- Confirm the portal has separate Setup and Configure Wi-Fi pages.
- Confirm the accent is a usable dropdown and both custom color pickers render correctly on iPhone.
- Change all five presets by phone; blank one slot and confirm it disappears from Send.
- Compose and send a short message in Morse, including a word space and a correction.
- Edit an existing preset in Morse, save it, reboot, and confirm it remains changed.
- Clear and save a preset; confirm the disabled slot remains available in Edit Presets.
- Let the configured timeout turn the backlight off, send a message from the other box, and confirm the display does not wake. Wake it with one press and confirm that wake-up press performs no navigation action.

## 3. Two-box messaging

Current status: basic room creation/join and bidirectional message exchange passed on 2026-08-15. Repeat this fuller regression before a release.

- Create a room on box A.
- Join the same room on box B.
- Send every preset in both directions.
- Confirm the sending box does not add its own broker echo to the inbox.
- Confirm received messages survive reboot.
- Mark a message read and confirm its state survives reboot.
- Re-send or inject the same message ID and confirm it is not duplicated.
- Change one box to a different room and confirm its old-room inbox is cleared.

## 4. Temporary-offline and reconnect

Current status: requires extended validation.

- With both boxes connected, disconnect box B from Wi-Fi without erasing configuration.
- Send a QoS 1 message from A.
- Restore B's connection and confirm the broker's persistent session delivers the queued message.
- Repeat with a longer disconnection within the broker's session-retention allowance.
- Confirm reconnect bursts do not duplicate stored messages or overflow normal use of the bounded receive queue.
- Confirm attempting to send from the disconnected box reports that the message was not sent; FriendBox has no outgoing offline queue.

## 5. OTA happy path

Current status: implemented, not yet physically validated end to end.

- Freeze and tag a known-good older release.
- Flash or install that version and confirm the Info screen reports it.
- Publish a newer numeric release tag.
- Confirm the device fetches the latest manifest, downloads into the inactive slot, verifies size/hash/image, reboots, and reports the new version.
- Watch the fourth Info page for `CHECKING`, `UPDATING`, or `UPDATE FAILED` while diagnosing the run.
- Confirm configuration and inbox data in NVS survive the update.
- Confirm an unchanged or older version is ignored.
- Confirm a deliberately bad manifest hash fails without changing the boot partition.

## 6. Rollback — required before relying on it

Current status: not validated.

Because rollback depends on the exact bootloader build:

1. Keep known-good firmware in the current slot.
2. OTA a deliberately test-only image that crashes or reboots before the app can mark itself valid.
3. Confirm the bootloader returns to the known-good slot.
4. If it does not, automatic rollback is not validated for this toolchain. Continue using SHA-verified A/B OTA, but use USB recovery or a separately validated rollback-enabled bootloader path.

Never publish the deliberately broken image as the normal latest release.
