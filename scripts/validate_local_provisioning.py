#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
assert re.search(r"^include/LocalServiceConfig\.h$", gitignore, re.MULTILINE), \
    "private LocalServiceConfig.h must be gitignored"

assert (ROOT / "include/LocalServiceConfig.example.h").is_file(), \
    "missing safe provisioning example"
service = (ROOT / "include/ServiceConfig.h").read_text(encoding="utf-8")
assert '__has_include("LocalServiceConfig.h")' in service
assert 'FRIEND_BOX_LOCAL_MQTT_PASSWORD ""' in service
assert 'kDefaultMqttTlsPort = 8883' in service

config_h = (ROOT / "src/config/DeviceConfig.h").read_text(encoding="utf-8")
config_cpp = (ROOT / "src/config/DeviceConfig.cpp").read_text(encoding="utf-8")
portal = (ROOT / "src/setup/SetupPortal.cpp").read_text(encoding="utf-8")
app = (ROOT / "src/app/App.cpp").read_text(encoding="utf-8")

assert 'String setupApName() const' in config_h
assert 'mqttPort{service::kDefaultMqttTlsPort}' in config_h
assert 'serviceSeededThisBoot()' in config_h
assert 'kServiceInitializedKey = "svcinit"' in config_cpp
assert 'bootstrapDefaultsAvailable()' in config_cpp
assert 'hasMeaningfulStoredServiceSettings(' in config_cpp
assert 'config.settings().setupApName()' in portal
assert '_config.settings().setupApName()' in app
assert 'SettingsDraft DeviceConfig::draft() const' in config_cpp
assert 'bool DeviceConfig::apply(SettingsDraft draft, RoomAction roomAction)' in config_cpp
assert 'mutableSettings' not in config_h

# The saved secret must not be rendered back into the captive portal.
assert '_password("mpass", "MQTT password", ""' in portal
assert '_initial.mqttPassword.c_str()' not in portal
assert 'if (!submittedPassword.isEmpty()) result.mqttPassword = submittedPassword;' in portal

# No real private file should be distributed by this patch.
assert not (ROOT / "include/LocalServiceConfig.h").exists(), \
    "private LocalServiceConfig.h must not be included in distributed patch"

print("local provisioning validation passed")
