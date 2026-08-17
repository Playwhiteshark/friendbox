#!/usr/bin/env python3
"""Fast repository-level checks that do not require PlatformIO or hardware."""
from __future__ import annotations

import csv
import os
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = [
    "platformio.ini",
    "partitions.csv",
    "include/BuildConfig.h",
    "include/HiveMqRootCa.h",
    "src/display/Display.cpp",
    "src/display/DisplayScreens.cpp",
    "src/messaging/MqttTransport.cpp",
    "src/ui/UiRenderer.cpp",
    "lib/FriendBoxCore/src/MorseComposer.cpp",
    "lib/FriendBoxCore/src/PresetCatalog.cpp",
    "src/update/OtaUpdater.cpp",
    ".github/workflows/build.yml",
    ".github/workflows/release.yml",
]
for name in required:
    assert (ROOT / name).is_file(), f"missing required file: {name}"

pio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
for expected in [
    "espressif32@6.5.0",
    "lilygo-t-display-s3",
    "TFT_eSPI@2.5.43",
    "WiFiManager@2.0.17",
    "ArduinoJson@7.4.3",
    "espMqttClient@1.7.3",
]:
    assert expected in pio, f"missing pinned build setting: {expected}"

partitions = (ROOT / "partitions.csv").read_text(encoding="utf-8")
for expected in ["otadata", "ota_0", "ota_1", "nvs"]:
    assert re.search(rf"^\s*{re.escape(expected)}\s*,", partitions, re.MULTILINE), f"missing partition {expected}"

build_config = (ROOT / "include/BuildConfig.h").read_text(encoding="utf-8")
assert "constexpr int kButtonPin = 1;" in build_config
assert "constexpr size_t kMaxStoredMessages = 50;" in build_config
assert "constexpr size_t kMqttRxQueueDepth = 16;" in build_config

repo_match = re.search(r'kGitHubRepository\s*=\s*"([^"]+)"', build_config)
assert repo_match, "kGitHubRepository setting not found"
configured_repo = repo_match.group(1)
ci_repo = os.environ.get("GITHUB_REPOSITORY", "").strip()
if ci_repo:
    assert configured_repo == ci_repo, (
        f"kGitHubRepository is {configured_repo!r}, but this GitHub repository is {ci_repo!r}; "
        "update include/BuildConfig.h before relying on OTA"
    )


# Resolve the custom partition table the same way the ESP32 partition tool does for blank offsets
# (data partitions align to 0x1000; app partitions align to 0x10000) and ensure it fits 16 MiB.
def parse_num(value: str) -> int:
    value = value.strip().lower()
    suffix = 1
    if value.endswith("k"):
        suffix, value = 1024, value[:-1]
    elif value.endswith("m"):
        suffix, value = 1024 * 1024, value[:-1]
    return int(value, 0) * suffix

def align(value: int, boundary: int) -> int:
    return (value + boundary - 1) // boundary * boundary

rows = []
next_offset = 0x9000
with (ROOT / "partitions.csv").open(newline="", encoding="utf-8") as f:
    for raw in csv.reader(line for line in f if not line.lstrip().startswith("#")):
        if not raw or not any(cell.strip() for cell in raw):
            continue
        raw += [""] * (6 - len(raw))
        name, ptype, subtype, offset_text, size_text, _flags = [cell.strip() for cell in raw[:6]]
        boundary = 0x10000 if ptype == "app" else 0x1000
        offset = parse_num(offset_text) if offset_text else align(next_offset, boundary)
        size = parse_num(size_text)
        assert offset % boundary == 0, f"partition {name} offset is not aligned"
        assert offset >= next_offset, f"partition {name} overlaps previous partition"
        rows.append((name, ptype, subtype, offset, size))
        next_offset = offset + size

assert next_offset <= 16 * 1024 * 1024, "partition table exceeds 16 MiB flash"
apps = {name: size for name, ptype, _subtype, _offset, size in rows if ptype == "app"}
assert apps.get("ota_0") == apps.get("ota_1") and apps.get("ota_0", 0) >= 4 * 1024 * 1024

# Validate the embedded public CA as an actual X.509 certificate.
ca_header = (ROOT / "include/HiveMqRootCa.h").read_text(encoding="utf-8")
match = re.search(r'R"PEM\((-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----)\n\)PEM"', ca_header, re.S)
assert match, "embedded MQTT CA PEM not found"
with tempfile.NamedTemporaryFile("w", suffix=".pem") as f:
    f.write(match.group(1) + "\n")
    f.flush()
    parsed = subprocess.run(
        ["openssl", "x509", "-in", f.name, "-noout", "-subject", "-issuer", "-enddate"],
        text=True,
        capture_output=True,
        check=True,
    ).stdout
    assert "CN = ISRG Root X1" in parsed or "CN=ISRG Root X1" in parsed

# Embedded code should not depend on C++ exceptions and TLS must never disable verification.
source_text = "\n".join(
    path.read_text(encoding="utf-8", errors="ignore")
    for root_name in ("src", "lib", "include")
    for path in (ROOT / root_name).rglob("*")
    if path.is_file()
)
assert "setInsecure(" not in source_text, "TLS verification was disabled"
assert not re.search(r"\btry\s*\{", source_text), "embedded source unexpectedly uses C++ exceptions"

# Preserve the feature-extension boundaries that keep future UI, Morse, pet,
# and setup work out of transport and storage internals.
assert "mutableSettings" not in source_text, "setup code must commit a SettingsDraft"
messaging_service = (ROOT / "src/messaging/MessagingService.h").read_text(encoding="utf-8")
assert "MessageStore" not in messaging_service, "transport/service must not own inbox persistence"
app_source = (ROOT / "src/app/App.cpp").read_text(encoding="utf-8")
assert "routeIncoming" in app_source, "incoming feature messages need an application routing boundary"
ui_header = (ROOT / "src/ui/Ui.h").read_text(encoding="utf-8")
assert "Intent handleAction" in ui_header, "UI navigation must emit intents rather than perform side effects"

# Guard against accidentally committing common token formats.
secret_patterns = {
    "GitHub PAT": r"\bghp_[A-Za-z0-9]{20,}\b",
    "GitHub fine-grained PAT": r"\bgithub_pat_[A-Za-z0-9_]{20,}\b",
    "AWS access key": r"\bAKIA[0-9A-Z]{16}\b",
}
for path in ROOT.rglob("*"):
    if not path.is_file() or ".git" in path.parts or path.suffix in {".zip", ".bin"}:
        continue
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        continue
    for label, pattern in secret_patterns.items():
        assert not re.search(pattern, text), f"possible {label} in {path.relative_to(ROOT)}"

print("project validation passed")
