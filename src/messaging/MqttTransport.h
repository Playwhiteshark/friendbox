#pragma once

#include <Arduino.h>
#include <atomic>
#include <espMqttClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "BuildConfig.h"
#include "config/DeviceConfig.h"

namespace friendbox::messaging {

enum class PayloadKind : uint8_t { Message, RoomMetadata };

class MqttTransport {
public:
    MqttTransport();
    bool begin(const config::Settings& settings);
    void update(bool wifiConnected);
    bool connected() const { return _connected.load(std::memory_order_relaxed); }
    bool publish(const String& payload);
    bool publishRoomMetadata(const String& payload);
    bool pollPayload(String& payload, PayloadKind& kind);
    void disconnect();

private:
    struct Packet {
        PayloadKind kind{PayloadKind::Message};
        uint16_t length{0};
        char payload[build::kMaxMqttPayloadBytes + 1]{};
    };

    struct Assembly {
        char data[build::kMaxMqttPayloadBytes + 1]{};
        size_t total{0};
        size_t received{0};
    };

    espMqttClientSecure _client;
    QueueHandle_t _rxQueue{nullptr};
    std::atomic_bool _connected{false};
    bool _configured{false};
    std::atomic_bool _subscriptionRetryNeeded{false};
    std::atomic<uint32_t> _lastConnectAttempt{0};

    // espMqttClient retains pointers to these strings; they must live as long as the client.
    String _host;
    String _username;
    String _password;
    String _clientId;
    String _messageTopic;
    String _metadataTopic;

    Assembly _messageAssembly;
    Assembly _metadataAssembly;

    void onConnect(bool sessionPresent);
    void onDisconnect(espMqttClientTypes::DisconnectReason reason);
    void onMessage(const espMqttClientTypes::MessageProperties& properties,
                   const char* topic, const uint8_t* payload,
                   size_t len, size_t index, size_t total);
    void assemble(PayloadKind kind, Assembly& assembly, const uint8_t* payload,
                  size_t len, size_t index, size_t total);
};

}  // namespace friendbox::messaging
