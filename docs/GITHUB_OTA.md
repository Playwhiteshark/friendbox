# GitHub release OTA

FriendBox does not `git pull`. Devices only install explicit release binaries.

## One-time repository setup

1. On GitHub, create an empty **public** repository, for example `friendbox`. Do not add a README/license there if this folder already contains them.
2. Edit `include/BuildConfig.h` and change `kGitHubRepository` to the exact public `owner/repository` slug.
3. From the root of this project, create the Git repository and push it:

```bash
git init
git add .
git commit -m "Initial FriendBox firmware"
git branch -M main
git remote add origin https://github.com/YOUR_NAME/friendbox.git
git push -u origin main
```

If the folder is already a Git repository, skip `git init` and add/commit only the changes you need.

4. Open the repository's **Actions** tab and confirm the `build` workflow passes. This is the first complete PlatformIO compile gate.
5. Flash a passing build to each FriendBox once by USB. Future tagged releases can update over the air.

No GitHub token is stored on the ESP32. GitHub Actions uses the repository's automatically-created workflow token when it creates a release.

## Publish an update

Use a numeric semantic version tag:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The release workflow:

1. validates the tag;
2. writes the version into `GeneratedVersion.h`;
3. runs host tests;
4. builds one universal firmware image;
5. calculates SHA-256 and size;
6. creates `manifest.json`;
7. creates a GitHub Release with `firmware.bin` and `manifest.json`.

Ordinary commits never update physical devices.

## Device discovery

Every configured FriendBox checks, after boot and then roughly every 12 hours:

`https://github.com/<owner>/<repo>/releases/latest/download/manifest.json`

It installs only if the manifest has schema 1, a newer numeric `MAJOR.MINOR.PATCH`, a GitHub release URL in the configured repository, a plausible size, and a 64-character SHA-256.

The firmware is streamed into the inactive OTA partition while SHA-256 is calculated. The boot partition is changed only after the size and hash match and ESP-IDF accepts the image.

## Rollback

The app contains the normal ESP-IDF post-boot validation hook (`esp_ota_mark_app_valid_cancel_rollback`) and the partition table contains `otadata`, `ota_0`, and `ota_1`.

Rollback is also a **bootloader compile-time setting**. The ESP32-S3 SDK configuration shipped by the pinned Arduino-ESP32 2.0.17 source has `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`, which is encouraging. PlatformIO still flashes a prebuilt framework bootloader, so the physical test in `HARDWARE_VALIDATION.md` remains the final proof for the exact packaged binary. Hash verification and A/B OTA work independently of that automatic rollback test.
