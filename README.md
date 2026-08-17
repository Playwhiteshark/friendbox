# FriendBox

FriendBox is a small internet-connected messenger for the **LILYGO T-Display-S3**. The same firmware runs on every box: each device gets its own stable ID and display name, then joins a room derived from a shared room code and password.

The current firmware provides:

- phone-based Wi-Fi and device setup;
- generated device identity plus a user-selected display name;
- room creation and joining with a six-character code and six-digit password;
- MQTT-over-TLS messaging with QoS 1, reconnect, and persistent broker sessions;
- five preset outgoing messages;
- a persistent 50-message inbox with unread state and duplicate suppression;
- a one-button interface with tap, hold, and long-hold actions;
- six persistent accent colors;
- an NTP-backed local clock; and
- GitHub Release OTA infrastructure with manifest, size, SHA-256, and A/B partition checks.

Two physical FriendBoxes have successfully completed setup, joined the same room, and exchanged messages through HiveMQ Cloud. The full OTA and automatic rollback paths still need end-to-end hardware validation; see [Validation status](docs/VALIDATION.md).

FriendBox deliberately has no application server, room database, user-account system, companion app, analytics service, or outgoing offline queue. Morse entry, reactions, pets/shared state, arbitrary typing, and direct messages are future features rather than hidden parts of v1.

## How it fits together

```text
button release
    -> ButtonDriver measures duration
    -> InputMapper classifies tap/hold/long hold
    -> Ui updates navigation state and emits an intent
    -> App performs any messaging/configuration side effect
    -> Display redraws the current screen

outgoing message
    -> MessagingService creates and validates Message JSON
    -> MqttTransport publishes to the room topic
    -> HiveMQ Cloud forwards it to the other boxes

incoming MQTT packet
    -> MqttTransport assembles and queues the payload
    -> MessagingService parses and ignores self-echoes
    -> App routes the message by its typed protocol kind
    -> MessageStore deduplicates and persists text messages in NVS
    -> Ui shows a notification and unread count
```

`App` is the coordinator. Display code does not know about MQTT, and MQTT code does not know which screen is open. Configuration and inbox persistence are separate NVS-backed components. Portable rules that can be tested without an ESP32 live in `lib/FriendBoxCore`.

For the full component map, startup lifecycle, storage layout, data flows, and extension points, read [Architecture and repository map](docs/ARCHITECTURE.md).

## Where things live

| Path | Responsibility |
| --- | --- |
| `src/main.cpp` | Arduino entry points; delegates to the single `App` instance |
| `src/app/` | Startup, main loop, and coordination between subsystems |
| `src/hardware/` | Board power/backlight setup and raw button timing |
| `src/input/` | Maps a released-button duration to a UI action |
| `src/ui/` | Testable screen/navigation state, UI intents, and rendering coordination |
| `src/display/` | TFT primitives, per-screen drawing, and RGB565 color mapping |
| `src/config/` | Device settings drafts, validation, identity, room derivation, and `fbconfig` NVS access |
| `src/setup/` | WiFiManager captive portal form; submits a settings draft |
| `src/network/` | Wi-Fi reconnect and NTP time |
| `src/messaging/` | Message JSON, MQTT transport, inbox logic, and `fbmsgs` NVS access |
| `src/update/` | GitHub manifest checks and A/B OTA installation |
| `src/util/` | ESP32-specific hashing helpers |
| `lib/FriendBoxCore/` | Host-testable rules and the mutable preset catalog, with no Arduino dependency |
| `include/BuildConfig.h` | Pins, timing thresholds, limits, intervals, and public repo slug |
| `include/ProductInfo.h` | User-facing name/title/setup identity; deliberately separate from protocol constants |
| `include/ServiceConfig.h` | Public MQTT bootstrap interface and safe empty defaults |
| `include/LocalServiceConfig.h` | Private local MQTT defaults; gitignored and never in public OTA builds |
| `platformio.ini` | Board, framework, flash mode, language standard, and pinned libraries |
| `partitions.csv` | A/B application slots, OTA metadata, and NVS layout |
| `tests/host/` | Desktop tests for portable core, presets, UI navigation, and manifest logic |
| `scripts/` | Local checks, project validation, provisioning validation, manifest generation |
| `.github/workflows/` | CI build and tagged-release pipelines |
| `docs/` | Architecture, provisioning, broker, OTA, validation, and references |

## Hardware

Target: **LILYGO T-Display-S3** with ESP32-S3 and 170×320 ST7789 display.

Wire one normally-open momentary button:

```text
GPIO1 ---- button ---- GND
```

The firmware uses `INPUT_PULLUP`, so released is HIGH and pressed is LOW. To use onboard KEY1 instead, change `kButtonPin` in `include/BuildConfig.h` from `1` to `14`.

The working, deliberately pinned toolchain is:

```ini
platform = espressif32@6.5.0
board = lilygo-t-display-s3
framework = arduino
board_build.flash_mode = dio
```

Platform `6.5.0` supplies Arduino-ESP32 `2.0.14`. The display uses `TFT_eSPI 2.5.43` with its T-Display-S3 `Setup206` configuration. These versions and DIO flash mode are intentional: later framework combinations and QIO caused display/reset or boot problems on the tested hardware.

## One-button controls

Actions are classified when the button is released:

- tap: less than 450 ms;
- hold: 450–1199 ms;
- long hold: 1200 ms or longer.

| Screen | Tap | Hold | Long hold |
| --- | --- | --- | --- |
| Idle | Open Inbox | Open Send | Open Info |
| Inbox | Next message | Mark current message read | Back |
| Send | Next preset | Send selected preset | Back |
| Info | Next info page | Cycle and save accent | Back |

Holding the button for about five seconds during startup forces setup mode. The low-level driver only reports duration; Morse can later use the same hardware with a different interpretation profile.

## Build and flash

Install VS Code with PlatformIO, or PlatformIO Core. Then:

```bash
pio run -e friendbox
pio run -e friendbox -t upload
pio device monitor
```

On the current Windows development machine, normal esptool stub uploads can disconnect partway through. The reliable recovery path is ROM download mode—hold BOOT, tap RST, release BOOT—and an equivalent esptool flash command using `--no-stub`. This is a USB/upload-tooling issue, not an application failure.

`include/BuildConfig.h` already points OTA at `Playwhiteshark/friendbox`. Change `kGitHubRepository` only when building a fork that will publish its own releases.

## First boot and setup

An unconfigured device creates an AP using its full stable device ID:

```text
FriendBox-Setup-<12-hex-character-device-id>
```

Connect a phone to that network and open the captive portal. Normal setup asks for:

- display name;
- room code/password, or both blank to create a room;
- accent color; and
- UTC offset, normally filled from the phone.

MQTT settings are under **Advanced service settings**. For boxes prepared by the developer, [local provisioning](docs/LOCAL_PROVISIONING.md) seeds those values into NVS during the first private USB build, so a friend normally never types them. A public build on an erased device has no private defaults and must be configured manually.

Hold the button for about five seconds during boot to reopen setup. Changing rooms clears the local inbox; changing only Wi-Fi, broker, name, time zone, or accent does not.

## Rooms and messages

A room is not created on a FriendBox server. Every box independently derives the same token from the same credentials:

```text
SHA256("friendbox-v1|" + roomCode + "|" + roomPassword)
```

The first 32 hexadecimal characters become the MQTT topic component:

```text
friendbox/v1/rooms/<room-token>/messages
```

Entering unused credentials therefore creates an empty room rather than returning “room not found.”

Current messages are JSON documents:

```json
{
  "v": 1,
  "id": "8b7c42f19d11-184",
  "sender_id": "8b7c42f19d11",
  "sender": "Alex",
  "ts": 1786281012,
  "type": "text",
  "text": "HELLO"
}
```

The protocol reserves `type` for future features, but the current parser intentionally accepts only `text`. QoS 1 can redeliver a packet, so the inbox rejects duplicate IDs; each sender also ignores its own publication when the broker sends it back through the shared topic.

There is no outgoing offline queue. If the sending box is disconnected, the UI reports that the message was not sent. A disconnected receiving box may receive broker-queued QoS 1 messages after reconnect because the MQTT client uses a stable ID and a persistent session; that behavior still needs longer-duration validation.

## Persistent storage

`DeviceConfig` stores settings in the `fbconfig` NVS namespace. `MessageStore` keeps up to 50 independently persisted message slots in `fbmsgs`, including sequence and unread state. When full, it replaces the oldest read message first; if all messages are unread, it replaces the oldest unread message.

Private compile-time broker defaults are a one-time bootstrap only. Once service settings exist in NVS, NVS is authoritative and later builds do not overwrite them.

## OTA and releases

Normal pushes and pull requests run tests and compile the complete firmware. Only a numeric tag such as `v0.1.0` creates `firmware.bin`, `manifest.json`, and a GitHub Release. No release tag should be created until the intended first-release behavior is frozen and the hardware OTA checklist is complete.

See [GitHub release OTA](docs/GITHUB_OTA.md) for the release flow and security checks.

## Tests and project documentation

Run all available local checks with:

```bash
./scripts/check.sh
```

The script runs host tests and repository validators, then also performs the PlatformIO firmware build when `pio` is installed. CI always installs PlatformIO and runs the full build.

- [Architecture and repository map](docs/ARCHITECTURE.md)
- [Local MQTT provisioning](docs/LOCAL_PROVISIONING.md)
- [HiveMQ Cloud setup](docs/HIVEMQ_SETUP.md)
- [GitHub release OTA](docs/GITHUB_OTA.md)
- [Validation status](docs/VALIDATION.md)
- [Physical validation checklist](docs/HARDWARE_VALIDATION.md)
- [Implementation references](docs/REFERENCES.md)

## Safe extension points

- New screen: add its `Screen` state and intent mapping in `Ui`, render routing in `UiRenderer`, and pixels in `DisplayScreens`.
- New persistent setting: add it to `SettingsDraft`/`Settings`, validate and persist it in `DeviceConfig`, expose it in `PortalForm`, then consume it from the relevant feature.
- Configurable presets: persist values through `DeviceConfig`, load them into `PresetCatalog`, and let phone/on-device editors call the catalog rather than editing send-screen code.
- Morse: add a compose screen and input profile; keep decoding/timing rules in `FriendBoxCore` where possible.
- New message type: extend `Message` validation/parsing and `App` handling; do not bury feature behavior in MQTT transport.
- New broker or transport: replace the implementation behind `MqttTransport`/`MessagingService` without coupling it to screens.

Preserve the main boundary: hardware and transports report facts; `Ui` emits navigation intents; `App` coordinates side effects and message routing; `Display` only renders; NVS-owning classes hide persistence details.
