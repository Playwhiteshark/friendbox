#include "MessagingService.h"
#include "BuildConfig.h"

namespace friendbox::messaging {

bool MessagingService::begin(config::DeviceConfig& deviceConfig) {
    _config = &deviceConfig;
    return _transport.begin(deviceConfig.settings());
}

void MessagingService::update(bool wifiConnected) {
    _transport.update(wifiConnected);
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
        Message parsed;
        if (!Message::fromJson(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), parsed)) continue;
        if (parsed.senderId == _config->settings().deviceId) continue;
        message = parsed;
        return true;
    }
    return false;
}

}  // namespace friendbox::messaging
