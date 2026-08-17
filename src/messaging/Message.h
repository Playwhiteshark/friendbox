#pragma once

#include <Arduino.h>

namespace friendbox::messaging {

enum class MessageType : uint8_t { Text };

const char* messageTypeName(MessageType type);
bool parseMessageType(const String& value, MessageType& out);

struct Message {
    uint8_t version{1};
    String id;
    String senderId;
    String sender;
    uint32_t timestamp{0};
    MessageType type{MessageType::Text};
    String text;

    String toJson() const;
    static bool fromJson(const uint8_t* payload, size_t length, Message& out);
    bool valid() const;
    bool validPayload() const;
};

}  // namespace friendbox::messaging
