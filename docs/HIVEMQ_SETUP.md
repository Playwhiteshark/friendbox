# HiveMQ Cloud setup

FriendBox v1 does not need a custom backend. The only server-side component is an MQTT broker.

## Recommended v1 broker

Use a free **HiveMQ Cloud Serverless** cluster.

1. Create a HiveMQ Cloud account and a Serverless cluster.
2. Copy the cluster hostname. FriendBox uses MQTT over TLS, normally port `8883`.
3. Open **Access Management** and create a username/password for FriendBox devices.
4. Give that credential publish and subscribe permission for:

   `friendbox/v1/rooms/+/messages`

5. Do not put the username or password in this repository.
6. During FriendBox phone setup, enter the hostname, port, username and password in the setup portal.

For the first handful of boxes, one broker credential is intentionally used. The room code/password still determines the private-ish room topic. If this ever becomes a product, replace the credential model behind `MqttTransport`; do not add a backend just for v1.

## TLS

`include/HiveMqRootCa.h` contains the self-signed **ISRG Root X1** certificate from Let's Encrypt. HiveMQ support currently identifies this Let's Encrypt root for HiveMQ Cloud TLS connections. If HiveMQ changes its certificate chain in the future, update this public CA certificate and ship a firmware release.

## Security boundary

This is a friend-project security model, not an account system. Anyone who gets the shared broker credential and a room code/password can access that room. Never use the public HiveMQ demo broker for real FriendBox messages.
