#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "Message.h"

namespace friendbox::messaging {

struct StoredMessage {
    Message message;
    bool unread{true};
    uint32_t sequence{0};
    uint8_t slot{0};
};

class MessageStore {
public:
    bool begin();
    void clear();
    bool add(const Message& message);
    bool containsId(const String& id) const;
    size_t count() const { return _messages.size(); }
    size_t unreadCount() const;
    const StoredMessage* at(size_t index) const;
    bool markRead(size_t index);
    size_t firstUnreadIndex() const;

private:
    Preferences _prefs;
    std::vector<StoredMessage> _messages;
    uint32_t _nextSequence{1};

    bool persist(const StoredMessage& stored);
    bool parseStored(const String& json, uint8_t slot, StoredMessage& out) const;
    String serializeStored(const StoredMessage& stored) const;
    void sortMessages();
};

}  // namespace friendbox::messaging
