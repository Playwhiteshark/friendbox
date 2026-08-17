#include "Message.h"
#include <ArduinoJson.h>
#include "BuildConfig.h"

namespace friendbox::messaging {

const char* messageTypeName(MessageType type) {
    switch (type) {
        case MessageType::Text: return "text";
    }
    return "";
}

bool parseMessageType(const String& value, MessageType& out) {
    if (value == "text") {
        out = MessageType::Text;
        return true;
    }
    return false;
}

bool Message::valid() const {
    return version == 1 && id.length() > 0 && id.length() <= 64 &&
           senderId.length() > 0 && senderId.length() <= 32 &&
           sender.length() > 0 && sender.length() <= 24 &&
           validPayload();
}

bool Message::validPayload() const {
    switch (type) {
        case MessageType::Text:
            return text.length() > 0 && text.length() <= build::kMaxTextBytes;
    }
    return false;
}

String Message::toJson() const {
    JsonDocument doc;
    doc["v"] = version;
    doc["id"] = id;
    doc["sender_id"] = senderId;
    doc["sender"] = sender;
    doc["ts"] = timestamp;
    doc["type"] = messageTypeName(type);
    doc["text"] = text;
    String output;
    serializeJson(doc, output);
    return output;
}

bool Message::fromJson(const uint8_t* payload, size_t length, Message& out) {
    if (!payload || length == 0 || length > build::kMaxMqttPayloadBytes) return false;
    JsonDocument doc;
    const auto error = deserializeJson(doc, payload, length);
    if (error) return false;

    Message parsed;
    parsed.version = doc["v"] | 0;
    parsed.id = String(doc["id"] | "");
    parsed.senderId = String(doc["sender_id"] | "");
    parsed.sender = String(doc["sender"] | "");
    parsed.timestamp = doc["ts"] | 0U;
    if (!parseMessageType(String(doc["type"] | ""), parsed.type)) return false;
    parsed.text = String(doc["text"] | "");
    if (!parsed.valid()) return false;
    out = parsed;
    return true;
}

}  // namespace friendbox::messaging
