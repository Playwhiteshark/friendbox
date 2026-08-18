#include "MessagingService.h"

#include <ArduinoJson.h>
#include "BuildConfig.h"

namespace friendbox::messaging {

bool MessagingService::begin(config::DeviceConfig& deviceConfig) {
    _config = &deviceConfig;
    _metadataPublishPending = deviceConfig.settings().ownsRoom();
    return _transport.begin(deviceConfig.settings());
}

void MessagingService::update(bool wifiConnected) {
    _transport.update(wifiConnected);
    const bool nowConnected = _transport.connected();
    if (!nowConnected) {
        _wasConnected = false;
        return;
    }
    if (!_wasConnected) {
        _wasConnected = true;
        if (_config && _config->settings().ownsRoom()) _metadataPublishPending = true;
    }
    if (_metadataPublishPending && publishRoomMetadata()) {
        _metadataPublishPending = false;
    }
}

bool MessagingService::publishRoomMetadata() {
    if (!_config || !_config->settings().ownsRoom() || _config->settings().roomName.isEmpty()) {
        return false;
    }
    JsonDocument doc;
    doc["v"] = 1;
    doc["type"] = "room_meta";
    doc["name"] = _config->settings().roomName;
    doc["owner_id"] = _config->settings().roomOwnerId;
    String payload;
    serializeJson(doc, payload);
    return _transport.publishRetained(payload);
}

bool MessagingService::handleRoomMetadata(const String& payload) {
    if (!_config || payload.isEmpty() || payload.length() > build::kMaxMqttPayloadBytes) return false;
    JsonDocument doc;
    if (deserializeJson(doc, payload)) return false;
    if (String(doc["type"] | "") != "room_meta") return false;
    if ((doc["v"] | 0) != 1) return true;
    const String name(doc["name"] | "");
    const String ownerId(doc["owner_id"] | "");
    if (_config->applyRoomMetadata(name, ownerId)) _roomMetadataChanged = true;
    return true;
}

bool MessagingService::sendText(const String& text, uint32_t timestamp) {
    if (!_config || !_transport.connected() || text.isEmpty() || text.length() > build::kMaxTextBytes) return false;
    Message message;
    message.id = _config->settings().deviceId + "-" + String(_config->nextOutgoingCounter());
    message.senderId = _config->settings().deviceId;
    message.sender = _config->settings().displayName;
    message.timestamp = timestamp;
    message.type = MessageType::Text;
    message.text = text;
    return message.valid() && _transport.publish(message.toJson());
}

bool MessagingService::pollIncoming(Message& message) {
    if (!_config) return false;
    String payload;
    while (_transport.pollPayload(payload)) {
        if (handleRoomMetadata(payload)) continue;
        Message parsed;
        if (!Message::fromJson(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), parsed)) continue;
        if (parsed.senderId == _config->settings().deviceId) continue;
        message = parsed;
        return true;
    }
    return false;
}

bool MessagingService::consumeRoomMetadataChanged() {
    const bool changed = _roomMetadataChanged;
    _roomMetadataChanged = false;
    return changed;
}

}  // namespace friendbox::messaging
