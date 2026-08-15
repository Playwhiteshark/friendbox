#pragma once

#include <Arduino.h>

namespace friendbox::messaging {

struct Message {
    uint8_t version{1};
    String id;
    String senderId;
    String sender;
    uint32_t timestamp{0};
    String type{"text"};
    String text;

    String toJson() const;
    static bool fromJson(const uint8_t* payload, size_t length, Message& out);
    bool valid() const;
};

}  // namespace friendbox::messaging
