# HiveMQ Cloud setup

FriendBox v1 has no custom backend. Its only server-side runtime dependency is an MQTT broker; the current deployment uses a free **HiveMQ Cloud Serverless** cluster.

## Cluster and permission setup

1. Create a HiveMQ Cloud account and a Serverless cluster.
2. Copy the cluster hostname. FriendBox uses MQTT over TLS, normally port `8883`.
3. In **Access Management**, create a FriendBox username/password.
4. Allow that credential to publish and subscribe to:

   ```text
   friendbox/v1/rooms/+/messages
   ```

5. Never commit the username or password to this repository.

For boxes prepared by the developer, follow [Local MQTT provisioning](LOCAL_PROVISIONING.md) so friends do not need to understand or type broker settings. A public build on a completely erased device can still accept the values under **Advanced service settings** in the setup portal.

One broker credential is intentionally shared by the first small set of boxes. The room code/password derives the room-specific topic. If FriendBox becomes a product, replace this credential model behind `MqttTransport`; do not spread account logic into the UI or message parser.

## MQTT behavior

FriendBox uses:

- TLS server authentication;
- a stable per-device client ID;
- MQTT 3.1.1 persistent sessions (`cleanSession(false)`);
- QoS 1 publish and subscribe;
- automatic reconnect; and
- a non-retained room message topic.

Persistent sessions can let HiveMQ queue subscribed QoS 1 messages while a receiving box is temporarily disconnected. FriendBox itself does not queue outgoing messages; sending while disconnected fails visibly.

## TLS

`include/HiveMqRootCa.h` contains the public ISRG Root X1 certificate used to authenticate the HiveMQ Cloud server certificate. If HiveMQ changes its certificate chain, update that public CA and publish a tested firmware release.

This MQTT trust path is separate from GitHub OTA HTTPS, which uses the ESP certificate bundle. Neither path disables certificate verification.

## Security boundary

This is a small friend-project security model, not an account system. Anyone who obtains both the broker credential and a room's code/password can derive and access that room topic. Do not use the public HiveMQ demo broker for real FriendBox messages, and do not treat the six-character room code/six-digit password as high-security credentials.
