# Local MQTT provisioning

FriendBox can bootstrap MQTT service credentials during the first private USB flash without putting those credentials in the public repository or public OTA releases.

## Files and ownership

| Location | Role |
| --- | --- |
| `include/LocalServiceConfig.example.h` | Public template committed to Git |
| `include/LocalServiceConfig.h` | Developer's real private defaults; excluded by `.gitignore` |
| `include/ServiceConfig.h` | Public interface that uses private defaults when present and safe empty defaults otherwise |
| `src/config/DeviceConfig.*` | One-time seeding rule and ongoing NVS ownership |
| `fbconfig` NVS namespace | Runtime source of truth after initialization |

The private header is a provisioning input, not a permanent configuration authority.

## One-time developer setup

1. Copy:

   ```text
   include/LocalServiceConfig.example.h
   ```

   to:

   ```text
   include/LocalServiceConfig.h
   ```

2. Put the real HiveMQ host, TLS port, username, and password in the private file.
3. Do **not** remove its `.gitignore` rule.
4. Build and flash the device locally over USB.

On the first boot of a device that has never had meaningful MQTT service settings, `DeviceConfig` copies complete private defaults into the `fbconfig` NVS namespace and sets the `svcinit` marker.

The precedence rule is:

```text
existing NVS service settings
    > one-time private compile defaults
    > empty public-build defaults
```

Existing host/username/password values from older firmware are treated as authoritative even when they predate `svcinit`; FriendBox records the marker without replacing them.

## After provisioning

NVS becomes authoritative. Future firmware does not consult the private defaults again for that device, and changing `LocalServiceConfig.h` then reflashing does not rotate an initialized device's credentials.

To intentionally change broker credentials, open setup mode and use **Advanced service settings**. The saved MQTT password is never rendered back into the setup page; leaving the password field blank preserves its current NVS value.

## Friend setup experience

The normal setup page still requires a complete FriendBox configuration. On a locally provisioned box, the service fields are already present, so a friend normally enters only:

- Wi-Fi credentials;
- display name;
- room information, or leaves both room fields blank to create one;
- accent; and
- time-zone offset.

Advanced service fields remain available for repairs without making broker configuration part of the normal product flow.

## Public builds and OTA

GitHub Actions does not have `include/LocalServiceConfig.h`, so public builds and release binaries contain no private bootstrap credential.

Normal OTA writes an application slot, not the NVS partition. MQTT credentials, device settings, and inbox records therefore survive an application update.

A completely erased device flashed from a public build has no private defaults. Its setup portal requires MQTT host, username, and password under **Advanced service settings**. That fallback is intentional.
