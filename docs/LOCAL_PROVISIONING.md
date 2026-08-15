# Local MQTT provisioning

FriendBox can bootstrap MQTT service credentials during the first USB flash without putting those credentials in the public repository or public OTA releases.

## One-time developer setup

1. Copy:

   `include/LocalServiceConfig.example.h`

   to:

   `include/LocalServiceConfig.h`

2. Put the real HiveMQ host, TLS port, username, and password in `LocalServiceConfig.h`.
3. Do **not** remove the `.gitignore` rule for this file.
4. Build and flash the device locally over USB.

On the first boot of a device that has never had MQTT service settings, `DeviceConfig` copies those values into the `fbconfig` NVS namespace and sets the `svcinit` marker.

After that, NVS is authoritative. The private compile-time values are not consulted again for that device. Existing broker settings are never silently replaced by later firmware builds.

## OTA behavior

Public GitHub Actions builds do not have `LocalServiceConfig.h`, so public OTA binaries contain no private MQTT defaults. This is intentional.

The MQTT credentials already stored in the device's NVS partition survive normal OTA application updates because FriendBox updates an OTA application partition, not the NVS data partition.

## Friend setup experience

The normal setup page still requires a complete FriendBox configuration, including valid MQTT service settings, before entering the main application.

On a locally provisioned box, MQTT settings are already in NVS before the setup portal opens. Your friend normally needs only Wi-Fi, their name, room information, and accent color.

MQTT fields remain available under **Advanced service settings** for repairs. The saved MQTT password is never inserted back into the setup HTML. Leaving that password field blank keeps the NVS password unchanged.

## Existing devices

If a device already has MQTT host/username/password values from an older FriendBox build, this firmware treats those values as authoritative and records `svcinit` without replacing them.

If you intentionally need to change a provisioned device's broker credentials, use **Advanced service settings**. Simply changing `LocalServiceConfig.h` and reflashing will not rotate an already initialized device.

## Public/fresh build behavior

A completely erased device flashed from a public GitHub build has no private defaults. Its setup portal therefore requires the MQTT values under Advanced service settings. That fallback is intentional.
