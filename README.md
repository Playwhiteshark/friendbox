# FriendBox

FriendBox is a deliberately small internet messenger for the **LILYGO T-Display-S3**. One universal firmware image runs on every box.

Version 1 provides:

- phone-based Wi-Fi/configuration portal;
- generated device identity and user display name;
- create/join FriendBox rooms with a six-character code and six-digit password;
- MQTT-over-TLS group messaging, QoS 1, persistent MQTT sessions, reconnect;
- five canned outgoing messages;
- 50-message persistent local inbox, unread state and QoS-1 deduplication;
- one external-button UI;
- selectable accent color (cyan, blue, green, orange, pink, purple);
- NTP clock;
- GitHub Release OTA with HTTPS, manifest checks, SHA-256 and A/B partitions;
- post-OTA validation hook for rollback-enabled bootloaders.

It intentionally does **not** include pets, Morse entry, arbitrary typing, direct messages, an app, a FriendBox backend, analytics, LVGL, or an outgoing offline queue.

## Hardware

Target board: **LILYGO T-Display-S3** (ESP32-S3, 170×320 ST7789 display).

Wire one normally-open momentary button:

```text
GPIO1 ---- button ---- GND
```

The firmware uses `INPUT_PULLUP`, so released = HIGH and pressed = LOW. To use the onboard KEY1 instead, change `kButtonPin` in `include/BuildConfig.h` from `1` to `14`.

The display wiring in `src/display/Display.cpp` follows LILYGO's official Arduino_GFX T-Display-S3 example. GPIO15 is driven HIGH before display startup to enable board peripherals; GPIO38 drives the backlight.

## One-button controls

Actions are classified only when the button is released:

- tap `< 450 ms`;
- hold `450–1199 ms`;
- long hold `>= 1200 ms`.

Idle:

- tap → Inbox
- hold → Send
- long hold → Info

Inbox:

- tap → next message
- hold → mark current message read
- long hold → back

Send:

- tap → next preset
- hold → send
- long hold → back

Info:

- tap → next info page
- hold → cycle the accent color and save it
- long hold → back

Holding the button for about five seconds **during startup** opens configuration mode. The low-level button driver only reports press/release duration, so a future Morse input profile can use different thresholds without changing hardware code.

## Install development tools

Recommended: VS Code + the PlatformIO extension. Or install PlatformIO Core and use its `pio` command.

The project pins:

```ini
platform = espressif32@7.0.1
board = lilygo-t-display-s3
framework = arduino
```

Libraries are also pinned in `platformio.ini` for reproducible builds.

## Before the first flash

### 1. Set the GitHub repository slug

Edit:

```cpp
// include/BuildConfig.h
constexpr const char* kGitHubRepository = "YOUR_GITHUB_NAME/friendbox";
```

This is public metadata, not a secret. If it remains `CHANGE_ME/CHANGE_ME`, OTA checking is disabled.

### 2. Set up the MQTT broker

Follow [`docs/HIVEMQ_SETUP.md`](docs/HIVEMQ_SETUP.md). Do **not** put MQTT credentials into source code.

### 3. Build

```bash
pio run -e friendbox
```

### 4. Flash over USB-C

```bash
pio run -e friendbox -t upload
```

Then watch logs if needed:

```bash
pio device monitor
```

If USB upload does not enter the bootloader automatically, use LILYGO's documented boot-mode procedure for the T-Display-S3 and try the upload again.

## First boot

On an unconfigured box, the display shows a setup AP named approximately:

```text
FriendBox-Setup-A1B2
```

Connect your phone to it. The WiFiManager captive portal lets you select normal Wi-Fi and asks for:

- your FriendBox display name;
- room code/password (leave **both blank** to create a room; fill both to join);
- MQTT host/port/username/password;
- accent name;
- UTC offset, populated from the phone by a small browser script where supported.

If a room is created, the device displays its generated code/password. The same values are always available later on the Info screen.

To change Wi-Fi, room, MQTT settings, or name, hold the button for about five seconds during boot. To create a fresh room from that portal, clear both room fields. Changing rooms clears the local message history; changing only Wi-Fi/broker/name does not.

## Room and protocol design

A room is not a server object. Both boxes derive:

```text
SHA256("friendbox-v1|" + groupCode + "|" + groupPassword)
```

and use the first 32 hex characters as a room token:

```text
friendbox/v1/rooms/<roomToken>/messages
```

Message payloads are small JSON documents:

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

QoS 1 is at-least-once delivery, so each incoming ID is checked against the local 50-message store before insertion. The sender also ignores its own room publication.

## Persistent inbox

Messages are stored in NVS in 50 independent slots. Each record has a local sequence number and read/unread flag. When all slots are occupied, the store overwrites the **oldest read** message first; only when every message is unread does it overwrite the oldest unread message. One arrival updates one message slot rather than rewriting a giant history blob.

## OTA

See [`docs/GITHUB_OTA.md`](docs/GITHUB_OTA.md).

Normal pushes only compile/test in CI. A tag such as:

```bash
git tag v0.1.0
git push origin v0.1.0
```

runs the release workflow, producing:

```text
firmware.bin
manifest.json
```

Devices periodically fetch the stable GitHub “latest release asset” manifest URL. OTA HTTPS uses ESP-IDF's X.509 root certificate bundle. Firmware is written to the inactive partition while its SHA-256 is calculated; the boot partition is changed only after size/hash/image validation succeeds.

## Validation status

See [`docs/VALIDATION.md`](docs/VALIDATION.md) for what was actually executed before packaging and what still requires a physical board/live services.

## Tests

Run the full local check script:

```bash
./scripts/check.sh
```

It runs the host core tests, manifest tests, repository/partition/TLS checks, and a complete PlatformIO firmware build when `pio` is installed. GitHub Actions always performs the complete PlatformIO firmware build on every push/PR. Hardware validation is intentionally small and high-value; follow [`docs/HARDWARE_VALIDATION.md`](docs/HARDWARE_VALIDATION.md).

## What is easy to change later

- **Button pin:** one constant in `BuildConfig.h`.
- **Button interpretation:** `InputMapper`, without touching the GPIO driver.
- **Colors:** palette mapping in `Display::accentColor()`; selected value stored in config.
- **Screen visuals:** only `src/display/Display.cpp` draws pixels/text.
- **Navigation:** `App::handleInput()` + lightweight `Ui` state.
- **Canned messages:** `kPresets` in `src/ui/Ui.cpp`.
- **Morse:** add a different input profile and screen; button driver already exposes durations.
- **New message types:** extend `Message` handling; transport/store do not need redesign.
- **Broker/library:** kept behind `MqttTransport`/`MessagingService`.

## Source/reference record

The design and implementation references are listed in [`docs/REFERENCES.md`](docs/REFERENCES.md). The code intentionally follows maintained libraries and vendor APIs instead of reimplementing captive portals, MQTT, JSON, TLS, or display drivers.
