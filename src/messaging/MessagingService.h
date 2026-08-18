#pragma once

#include "MqttTransport.h"
#include "Message.h"
#include "config/DeviceConfig.h"

namespace friendbox::messaging {

class MessagingService {
public:
    bool begin(config::DeviceConfig& deviceConfig);
    void update(bool wifiConnected);
    bool connected() const { return _transport.connected(); }
    bool sendText(const String& text, uint32_t timestamp);
    bool pollIncoming(Message& message);
    bool consumeRoomMetadataChanged();
    void disconnect() { _transport.disconnect(); }

private:
    config::DeviceConfig* _config{nullptr};
    MqttTransport _transport;
    bool _wasConnected{false};
    bool _metadataPublishPending{false};
    bool _roomMetadataChanged{false};

    bool publishRoomMetadata();
    void handleRoomMetadata(const String& payload);
};

}  // namespace friendbox::messaging
