#include "MqttTransport.h"

#include <WiFi.h>
#include "BuildConfig.h"
#include "HiveMqRootCa.h"

namespace friendbox::messaging {

MqttTransport::MqttTransport() : _client(espMqttClientTypes::UseInternalTask::YES) {}

bool MqttTransport::begin(const config::Settings& settings) {
    if (!settings.hasRoom() || !settings.hasBroker()) return false;
    _host = settings.mqttHost;
    _username = settings.mqttUsername;
    _password = settings.mqttPassword;
    _clientId = "friendbox-" + settings.deviceId;
    _topic = "friendbox/v1/rooms/" + settings.roomToken + "/messages";

    _rxQueue = xQueueCreate(build::kMqttRxQueueDepth, sizeof(Packet));
    if (!_rxQueue) return false;

    _client.setCACert(FRIEND_BOX_HIVEMQ_ROOT_CA);
    _client.setServer(_host.c_str(), settings.mqttPort);
    if (!_username.isEmpty()) _client.setCredentials(_username.c_str(), _password.c_str());
    _client.setClientId(_clientId.c_str());
    _client.setCleanSession(false);
    _client.setKeepAlive(30);
    _client.setTimeout(10);

    _client.onConnect([this](bool sessionPresent) { onConnect(sessionPresent); });
    _client.onDisconnect([this](espMqttClientTypes::DisconnectReason reason) { onDisconnect(reason); });
    _client.onMessage([this](const espMqttClientTypes::MessageProperties& properties,
                             const char* topic, const uint8_t* payload,
                             size_t len, size_t index, size_t total) {
        onMessage(properties, topic, payload, len, index, total);
    });

    _configured = true;
    return true;
}

void MqttTransport::update(bool wifiConnected) {
    if (!_configured) return;
    if (_subscriptionRetryNeeded.exchange(false, std::memory_order_relaxed)) {
        _client.disconnect(true);
        return;
    }
    if (!wifiConnected || _connected.load(std::memory_order_relaxed)) return;
    if (!_client.disconnected()) return;
    const uint32_t now = millis();
    const uint32_t lastAttempt = _lastConnectAttempt.load(std::memory_order_relaxed);
    if (lastAttempt != 0 && now - lastAttempt < build::kMqttReconnectMs) return;
    _lastConnectAttempt.store(now, std::memory_order_relaxed);
    _client.connect();
}

void MqttTransport::onConnect(bool sessionPresent) {
    (void)sessionPresent;
    _connected.store(true, std::memory_order_relaxed);
    if (_client.subscribe(_topic.c_str(), 1) == 0) {
        _connected.store(false, std::memory_order_relaxed);
        _subscriptionRetryNeeded.store(true, std::memory_order_relaxed);
    }
}

void MqttTransport::onDisconnect(espMqttClientTypes::DisconnectReason reason) {
    (void)reason;
    _connected.store(false, std::memory_order_relaxed);
    _lastConnectAttempt.store(millis(), std::memory_order_relaxed);
}

void MqttTransport::onMessage(const espMqttClientTypes::MessageProperties& properties,
                              const char* topic, const uint8_t* payload,
                              size_t len, size_t index, size_t total) {
    (void)properties;
    if (!topic || _topic != topic || total == 0 || total > build::kMaxMqttPayloadBytes) return;

    if (index == 0) {
        _assemblyTotal = total;
        _assemblyReceived = 0;
        memset(_assembly, 0, sizeof(_assembly));
    }
    if (_assemblyTotal != total || index != _assemblyReceived || index + len > build::kMaxMqttPayloadBytes) {
        _assemblyTotal = 0;
        _assemblyReceived = 0;
        return;
    }

    memcpy(_assembly + index, payload, len);
    _assemblyReceived += len;
    if (_assemblyReceived == _assemblyTotal && _rxQueue) {
        Packet packet;
        packet.length = static_cast<uint16_t>(_assemblyTotal);
        memcpy(packet.payload, _assembly, _assemblyTotal);
        packet.payload[_assemblyTotal] = '\0';
        // The callback runs on espMqttClient's worker task. A short bounded wait lets
        // the main loop drain a reconnect burst without allocating a large queue.
        xQueueSend(_rxQueue, &packet, pdMS_TO_TICKS(25));
        _assemblyTotal = 0;
        _assemblyReceived = 0;
    }
}

bool MqttTransport::publish(const String& payload) {
    if (!_connected.load(std::memory_order_relaxed) || payload.isEmpty() || payload.length() > build::kMaxMqttPayloadBytes) return false;
    return _client.publish(_topic.c_str(), 1, false,
                           reinterpret_cast<const uint8_t*>(payload.c_str()),
                           payload.length()) != 0;
}

bool MqttTransport::pollPayload(String& payload) {
    if (!_rxQueue) return false;
    Packet packet;
    if (xQueueReceive(_rxQueue, &packet, 0) != pdTRUE) return false;
    payload = String(packet.payload).substring(0, packet.length);
    return true;
}

void MqttTransport::disconnect() {
    if (!_configured) return;
    _client.disconnect(false);
    _connected.store(false, std::memory_order_relaxed);
}

}  // namespace friendbox::messaging
