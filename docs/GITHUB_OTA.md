# GitHub release OTA

FriendBox does not run Git or update from ordinary commits. Devices install only compiled assets from explicit GitHub Releases.

## Repository and build configuration

The public OTA source is configured in `include/BuildConfig.h`:

```cpp
constexpr const char* kGitHubRepository = "Playwhiteshark/friendbox";
```

Forks must change that slug before relying on their own releases. `scripts/validate_project.py` checks that the configured value matches `GITHUB_REPOSITORY` during CI.

The release build uses the same pinned firmware toolchain as normal CI:

- PlatformIO `espressif32@6.5.0`;
- Arduino-ESP32 2.0.14;
- LILYGO T-Display-S3 target;
- DIO flash mode; and
- the exact library versions in `platformio.ini`.

Public CI builds do not contain `include/LocalServiceConfig.h`. MQTT credentials already stored in the NVS partition survive normal A/B application updates.

## Ordinary pushes

Every push and pull request runs `.github/workflows/build.yml`, which performs host tests, repository validation, and a complete PlatformIO build. A passing normal build does not publish firmware to devices.

## Publish an update

The release workflow accepts only a numeric semantic-version tag:

```bash
git tag v0.1.0
git push origin v0.1.0
```

`.github/workflows/release.yml` then:

1. validates `vMAJOR.MINOR.PATCH`;
2. writes the tag version into `include/GeneratedVersion.h` for that build;
3. runs host tests and repository validation;
4. builds one universal firmware image;
5. copies it to `dist/firmware.bin`;
6. generates `dist/manifest.json` with version, size, SHA-256, and release URL; and
7. creates a GitHub Release containing both assets.

The committed development header remains `0.1.0-dev`; the release workflow's generated value belongs to the tagged build. Release `v0.1.0` is the baseline image used for the first physical OTA test.

No GitHub token is stored on a FriendBox. The workflow uses GitHub's short-lived repository token to create the release.

## Device discovery

After the initial boot delay, and then approximately every 12 hours, a configured device with Wi-Fi and valid network time requests:

```text
https://github.com/<owner>/<repo>/releases/latest/download/manifest.json
```

The request uses HTTPS certificate verification through the public-root bundle embedded from `data/cert/x509_crt_bundle.bin`. Arduino-ESP32 2.x does not populate its TLS callback automatically, so `OtaUpdater::begin()` installs the embedded bundle before any request. Its HTTP transmit buffer is also enlarged from the ESP-IDF default because GitHub release assets redirect to long signed URLs. The device accepts a manifest only when:

- `schema` is `1`;
- `version` is strict numeric `MAJOR.MINOR.PATCH`;
- the version is newer than the running firmware;
- the firmware URL begins with this repository's GitHub Release path;
- `sha256` is 64 hexadecimal characters; and
- the declared size is plausible and fits the inactive OTA partition.

The fourth Info page reports the current updater state as `IDLE`, `CHECKING`, `UPDATING`, or `UPDATE FAILED`. A failed check also shows the exact stage plus any HTTP/ESP error code, and the same detail is written to serial with the available heap. It is diagnostic status only; updates remain automatic.

## Installation

Firmware is streamed into the inactive application slot while SHA-256 is calculated. FriendBox changes the boot partition only after all of these succeed:

- HTTP request and status;
- exact downloaded byte count;
- exact SHA-256 match;
- ESP-IDF image finalization; and
- boot-partition selection.

The active firmware and NVS are not overwritten during the download. A failed check aborts the inactive-slot write and leaves the current boot target unchanged.
After selecting the verified boot partition, FriendBox restarts automatically without waiting for USB serial output to drain. This keeps an absent or disconnected serial monitor from blocking the reboot.

## Partition layout

`partitions.csv` defines:

| Partition | Offset | Size |
| --- | ---: | ---: |
| `otadata` | `0xE000` | `0x2000` |
| `ota_0` | `0x10000` | `0x600000` |
| `ota_1` | `0x610000` | `0x600000` |
| `nvs` | `0xC10000` | `0x40000` |

The two 6 MiB application slots support A/B updates; the separate NVS partition retains configuration and messages.

## Post-update validation and rollback

On boot, `OtaUpdater` checks whether the running partition is pending verification. After the application has been healthy for five seconds, it calls `esp_ota_mark_app_valid_cancel_rollback()`.

That application hook and the A/B partition layout are necessary but do not, by themselves, prove automatic rollback for the exact prebuilt bootloader flashed by this PlatformIO framework package. Complete the destructive rollback test in [Physical validation checklist](HARDWARE_VALIDATION.md) before relying on it.
