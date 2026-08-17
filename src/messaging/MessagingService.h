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
    void disconnect() { _transport.disconnect(); }

private:
    config::DeviceConfig* _config{nullptr};
    MqttTransport _transport;
};

}  // namespace friendbox::messaging
