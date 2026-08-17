# Architecture and repository map

This document answers two questions: **where does a FriendBox concern belong?** and **how does data move through the firmware?**

FriendBox is one ESP32-S3 firmware image plus external services used for transport and releases. There is no FriendBox backend.

## System boundary

```text
phone during setup
    -> WiFiManager captive portal on the FriendBox
    -> DeviceConfig
    -> ESP32 NVS

physical button
    -> input/UI/application layers
    -> Message JSON
    -> MQTT over TLS
    -> HiveMQ Cloud room topic
    -> another FriendBox
    -> persistent local inbox

Git tag
    -> GitHub Actions
    -> firmware.bin + manifest.json in a GitHub Release
    -> HTTPS OTA check from each FriendBox
```

HiveMQ Cloud transports packets but does not know FriendBox users or rooms. GitHub hosts source, CI, and explicit release assets. Each ESP32 owns its device settings, room credentials, and inbox.

## Dependency direction

`src/main.cpp` creates one `App`. `App` owns the firmware subsystems and is the only place that coordinates them.

```text
main.cpp
  -> App
       -> hardware + input
       -> Ui -> intents -> App
       -> UiRenderer -> Display
       -> DeviceConfig + SetupPortal
       -> WifiService + TimeService
       -> MessagingService -> MqttTransport
       -> MessageStore
       -> OtaUpdater
       -> PresetCatalog

ESP32-facing components
  -> FriendBoxCore for portable rules
```

The important boundary is not the folder diagram itself:

- `Display` receives already-decided content and draws it. It does not connect to MQTT or mutate configuration.
- `ButtonDriver` debounces GPIO and reports a release duration. It does not decide what a hold means.
- `InputMapper` classifies the duration. `Ui` maps it against the current screen and returns an intent; `App` executes side effects.
- `MqttTransport` moves bounded byte payloads. It does not interpret screens or persist messages.
- `MessagingService` creates/parses messages and rejects self-echoes. It does not own inbox persistence.
- `MessageStore` owns inbox persistence and retention.
- `DeviceConfig` owns persisted settings and room identity. Setup code edits a copy and commits it through `DeviceConfig::apply()`.
- `Ui` owns page/selection/notice state and emits intents. `UiRenderer` converts that state plus runtime data into draw calls.
- `App` owns cross-component behavior and routes typed incoming messages to their owning feature/store.

## Repository map

### Root and build files

| Path | What belongs there |
| --- | --- |
| `platformio.ini` | Exact ESP32 platform, board, flash mode, C++ version, and library pins |
| `partitions.csv` | OTA metadata, two application slots, and NVS partition |
| `README.md` | Product overview, setup/build entry point, and links to deeper docs |
| `.gitignore` | Generated build output and private local configuration exclusions |

The known-good build is `espressif32@6.5.0`, Arduino-ESP32 2.0.14, `TFT_eSPI@2.5.43`, and DIO flash mode. Treat those pins as compatibility constraints until a newer combination is tested on the actual board.

### Configuration headers

| Path | Responsibility |
| --- | --- |
| `include/BuildConfig.h` | GPIO pins, input thresholds, size limits, reconnect/check intervals, NTP servers, OTA enable flag, repository slug |
| `include/ProductInfo.h` | User-facing product name, display title, setup title/AP prefix, OTA user-agent identity |
| `include/GeneratedVersion.h` | Development version; release CI rewrites it from the numeric Git tag |
| `include/ServiceConfig.h` | Public bootstrap contract, default MQTT TLS port, and safe fallbacks |
| `include/LocalServiceConfig.example.h` | Committable template for local provisioning |
| `include/LocalServiceConfig.h` | Real private defaults on a developer machine; ignored by Git and absent from public builds |
| `include/HiveMqRootCa.h` | Public CA used to authenticate the MQTT server certificate |

Use `BuildConfig.h` for engineering constants. Use `Settings`/`DeviceConfig` for user/device values that survive reboot. Do not place secrets in `BuildConfig.h` or a workflow.

### Runtime source

| Area | Main files | Responsibility |
| --- | --- | --- |
| Application | `src/main.cpp`, `src/app/App.*` | Construct subsystems, perform startup, run the cooperative loop, coordinate all cross-component behavior |
| Hardware | `src/hardware/Board.*`, `ButtonDriver.*` | Enable board power/backlight; debounce and measure raw presses |
| Input | `src/input/InputMapper.*` | Convert release duration to `Tap`, `Hold`, or `LongHold` |
| UI state | `src/ui/Ui.*` | Portable current screen, indices, notices, action-to-intent mapping |
| UI rendering | `src/ui/UiRenderer.*` | Combine UI state with runtime data and select a screen renderer |
| Rendering | `src/display/Display.cpp`, `DisplayScreens.cpp` | TFT primitives/theme and per-screen pixels/footer hints |
| Device config | `src/config/DeviceConfig.*` | Stable ID, editable drafts, centralized validation/commit, room derivation, `fbconfig` NVS, one-time service seeding |
| Setup | `src/setup/SetupPortal.*` | WiFiManager `PortalForm`, submitted draft parsing, room create/join request |
| Network | `src/network/WifiService.*`, `TimeService.*` | Station reconnect and NTP-derived time formatting |
| Messages | `src/messaging/Message.*` | Version-1 JSON schema and validation; currently `text` only |
| Inbox | `src/messaging/MessageStore.*` | `fbmsgs` NVS slots, order, unread state, duplicate lookup, retention |
| Messaging | `src/messaging/MessagingService.*` | Outgoing IDs/JSON; incoming parse and self-echo rejection only |
| MQTT | `src/messaging/MqttTransport.*` | TLS client, persistent session, topic subscription, QoS 1 publish, bounded receive queue |
| Updates | `src/update/OtaUpdater.*` | Manifest retrieval, version/URL/size/hash checks, inactive-slot write, boot selection |
| Utilities | `src/util/Hash.*` | ESP32 SHA-256 helpers used for room tokens and validation |

### Portable core and tests

`lib/FriendBoxCore/src/` contains logic that compiles without Arduino:

- accent parsing and cycling;
- button timing classification;
- room-code and room-password validation;
- numeric semantic-version comparison; and
- inbox replacement-slot selection; and
- the fixed-capacity, runtime-replaceable `PresetCatalog` with current defaults.

`Ui` navigation is also intentionally Arduino-free and host-tested. Host tests live under `tests/host/`. Repository checks and build/release helpers live under `scripts/`. Keep new pure rules in `FriendBoxCore` when they do not need GPIO, NVS, Wi-Fi, Arduino `String`, or another ESP32 API.

### Automation and documentation

| Path | Responsibility |
| --- | --- |
| `.github/workflows/build.yml` | On every push/PR: host tests, repository validators, complete PlatformIO build |
| `.github/workflows/release.yml` | On `vMAJOR.MINOR.PATCH`: set firmware version, test/build, generate manifest, publish release assets |
| `docs/LOCAL_PROVISIONING.md` | Private MQTT-default bootstrap and NVS precedence |
| `docs/HIVEMQ_SETUP.md` | Broker permissions, TLS, and security boundary |
| `docs/GITHUB_OTA.md` | Release and on-device update path |
| `docs/VALIDATION.md` | What has actually passed and what remains |
| `docs/HARDWARE_VALIDATION.md` | Repeatable physical test checklist |
| `docs/REFERENCES.md` | Vendor/library sources behind implementation choices |

## Startup lifecycle

`App::begin()` performs startup in this order:

1. Start serial output and enable board power/backlight.
2. Initialize the TFT and show the boot screen.
3. Start the button driver and initialize UI state.
4. Open `DeviceConfig`; create the stable device ID if missing; load NVS; optionally seed private service defaults once.
5. Open `MessageStore` and rebuild the ordered in-memory inbox view from NVS slots.
6. Detect a five-second startup hold.
7. Run the setup portal if forced or if configuration is incomplete.
8. Start Wi-Fi, time, MQTT messaging, and OTA services.
9. Enter the normal update loop on the Idle screen.

If a forced maintenance portal fails but the previous settings remain complete, FriendBox keeps the known-good configuration. A first-time incomplete setup cannot enter the normal application.

## Main loop

`App::update()` is cooperative and intentionally small:

1. Update button debouncing.
2. Update Wi-Fi reconnect.
3. Start/maintain time when Wi-Fi is available.
4. Update MQTT connection state.
5. Expire temporary UI notices.
6. Parse and route one accepted incoming message in the application layer.
7. Poll one button-release event, update UI state, and execute any returned intent.
8. Let OTA schedule a background check when due.
9. Mark UI dirty when network, unread count, or clock text changes.
10. Render only when `Ui` is dirty.

The MQTT library runs callbacks on its internal task. `MqttTransport` only assembles a bounded payload and places it on a FreeRTOS queue there; JSON parsing, NVS writes, and UI changes happen later through the main application loop.

## Input and UI flow

```text
GPIO LOW/HIGH
  -> ButtonDriver debounce
  -> ButtonRelease{heldMs}
  -> InputMapper
  -> ButtonAction
  -> Ui::handleAction(current screen, action)
  -> Ui state change + optional Intent
  -> App::executeIntent(message/configuration side effects)
  -> UiRenderer -> Display
```

This separation is what makes Morse feasible. A Morse mode can interpret raw durations and pauses differently without turning the GPIO driver into a UI state machine.

The current screens are `Idle`, `Inbox`, `Send`, and `Info`. `Ui` owns selected indexes and temporary notices without depending on Arduino, NVS, MQTT, or the display. `UiRenderer` selects already-decided content; `Display` owns every pixel and font/color decision. `PresetCatalog` owns send-menu values, so later editors do not need to modify screen navigation.

## Configuration and setup flow

The stable `deviceId` is a 12-character hexadecimal value derived from the ESP32 eFuse MAC. It is separate from the human-readable `displayName` and is used in:

- setup AP name: `FriendBox-Setup-<deviceId>`;
- MQTT client ID: `friendbox-<deviceId>`; and
- outgoing message ID: `<deviceId>-<persistent-counter>`.

User-configurable settings are stored in the `fbconfig` NVS namespace. They include name, room code/password/token, MQTT service values, time-zone offset, accent, service-initialized marker, and the outgoing counter.

The captive portal starts from `DeviceConfig::draft()`. `PortalForm` edits that copy, and `DeviceConfig::apply()` trims, validates, derives the room token, persists, and only then replaces live settings. This transaction boundary is where future phone-configurable values—including preset messages—should enter the system. On-device editors should submit through the same configuration owner rather than writing NVS themselves.

Room creation generates credentials locally. Room joining validates supplied credentials. Both derive:

```text
SHA256("friendbox-v1|" + groupCode + "|" + groupPassword)
```

and retain the first 32 hex characters as the room token. No request is sent to a room database. Changing the derived room token clears the inbox so messages from two rooms are not mixed.

Private local MQTT defaults have one job: seed a blank device during its first developer USB flash. The precedence is:

```text
existing NVS service settings
    > one-time private compile defaults
    > empty public-build defaults
```

After initialization, reflashing a different `LocalServiceConfig.h` does not rotate credentials. The maintenance portal is the intentional repair path.

## Messaging flow

### Outgoing

1. `App` asks `MessagingService::sendText()` to send the selected preset.
2. The service creates a unique ID using the device ID and persisted counter.
3. `Message::valid()` enforces schema version 1, lengths, and the typed `MessageType::Text` payload rules.
4. The message is serialized to JSON.
5. `MqttTransport` publishes to `friendbox/v1/rooms/<token>/messages` at QoS 1, non-retained.
6. If MQTT is disconnected, publish fails immediately and the UI says the message was not sent.

There is intentionally no local outgoing queue.

### Incoming

1. The secure MQTT client receives one or more payload chunks on its worker task.
2. `MqttTransport` rejects the wrong topic, zero/oversized payloads, and inconsistent chunk ordering.
3. A complete payload is copied into a bounded FreeRTOS queue.
4. `MessagingService` parses/validates JSON and rejects the local sender ID in the main loop.
5. `App::routeIncoming()` dispatches on `MessageType`.
6. The text route asks `MessageStore` to reject duplicates and persist the message as unread.
7. `App` shows a temporary sender/message notification; the unread counter changes.

MQTT uses a stable client ID and `cleanSession(false)`, allowing the broker to retain the subscription/session. QoS 1 means duplicate delivery is legal, so application-level IDs and deduplication are required even when everything is healthy.

## Inbox storage

`MessageStore` uses the `fbmsgs` NVS namespace and up to 50 keys (`m00` through `m49`) rather than one monolithic history blob. Each record stores the message plus a local sequence number and unread flag.

On boot, valid slots are loaded and sorted by sequence. Corrupt records are removed. On insertion:

1. use an unused slot;
2. otherwise replace the oldest read message;
3. otherwise replace the oldest unread message.

Marking a message read rewrites only that slot. Changing rooms clears the namespace.

## TLS boundaries

There are two separate HTTPS/TLS paths:

- MQTT authenticates HiveMQ using the embedded public ISRG Root X1 certificate in `include/HiveMqRootCa.h`.
- GitHub OTA HTTPS uses the ESP certificate bundle through `arduino_esp_crt_bundle_attach`.

Neither path disables certificate verification. Broker username/password values live in NVS after setup and must never be committed to the public repository.

## OTA flow

The app checks after an initial delay and then approximately every 12 hours when Wi-Fi and network time are valid:

1. Fetch the latest-release `manifest.json` over HTTPS.
2. Require schema 1, strict numeric `MAJOR.MINOR.PATCH`, a URL under this repository's releases, a plausible size, and a SHA-256 value.
3. Ignore versions that are not newer than the running firmware.
4. Stream the firmware into the inactive OTA partition while calculating SHA-256.
5. Require HTTP success, exact byte count, matching hash, and a valid ESP image.
6. Select the new boot partition and restart.
7. After a healthy boot delay, call the ESP-IDF validation hook.

The partition table contains `otadata`, equal 6 MiB `ota_0`/`ota_1` slots, and NVS. Application-level validation infrastructure exists, but automatic bootloader rollback is not considered proven until the destructive hardware test passes.

## Adding features without breaking the boundaries

### New screen

1. Add a `Screen` value and page/selection state in `Ui`.
2. Map button actions to state changes and explicit `Intent` values in `Ui`.
3. Execute cross-component intents in `App::executeIntent()`.
4. Add render selection in `UiRenderer` and pixels in `DisplayScreens`.
5. Keep network/storage logic out of the draw function.

### New persistent option

1. Add the editable value to `SettingsDraft` and the committed value to `Settings`.
2. Load/validate/save it through centralized keys in `DeviceConfig`.
3. Add its phone field to `PortalForm` if user-configurable.
4. Consume it from the owning feature.
5. Add migration-safe defaults and host-testable parsing where possible.

### New message type

1. Define its wire schema and size limits.
2. Extend `Message` parsing/validation without weakening existing text validation.
3. Route the parsed enum value in `App::routeIncoming()`.
4. Persist only the data the inbox needs, or introduce a separate state store when the feature is not an inbox message.
5. Do not add feature-specific behavior to `MqttTransport`.

### Morse

Morse should be a dedicated compose screen and input interpretation profile. Put alphabet/timing/decoding rules in `FriendBoxCore` so they can be tested on a computer. Keep the raw `ButtonDriver` unchanged.

The seam is `App::handleButtonRelease()`: normal screens pass the release through `InputMapper` to `Ui`; a future Morse compose controller can instead consume raw duration and pause timing, then emit compose/send intents.

### Shared pet or synchronized state

Treat this as its own feature/state module and protocol type, not as fields stapled onto `MqttTransport` or `MessageStore`. MQTT is the carrier, not the product model.

The current routing boundary already prevents non-inbox events from being automatically persisted as text. A future pet module should own pet state and consume its `MessageType` branch from `App::routeIncoming()`.

## Architectural invariants

- One universal firmware image; identity and user settings are data.
- No FriendBox backend is required for v1.
- Hardware drivers report facts; `Ui` decides navigation; `App` decides cross-component effects.
- Rendering stays independent from networking and persistence.
- MQTT callbacks do bounded transport work only; app logic stays in the main flow.
- NVS is authoritative after one-time provisioning.
- Public builds contain no private MQTT credential defaults.
- Current protocol validation accepts only implemented message types.
- Toolchain pins and DIO flash mode change only after physical regression testing.
- Validation docs distinguish automated, physically proven, and still-pending behavior.
