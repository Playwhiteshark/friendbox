#include "MessageStore.h"

#include <ArduinoJson.h>
#include <algorithm>
#include "BuildConfig.h"
#include "FriendBoxCore.h"

namespace friendbox::messaging {
namespace {
constexpr const char* kNamespace = "fbmsgs";
void slotKey(uint8_t slot, char (&key)[5]) { snprintf(key, sizeof(key), "m%02u", slot); }
}

bool MessageStore::begin() {
    if (!_prefs.begin(kNamespace, false)) return false;
    _messages.clear();
    _nextSequence = _prefs.getUInt("nextseq", 1);
    if (_nextSequence == 0) _nextSequence = 1;

    for (uint8_t slot = 0; slot < build::kMaxStoredMessages; ++slot) {
        char key[5];
        slotKey(slot, key);
        const String raw = _prefs.getString(key, "");
        if (raw.isEmpty()) continue;
        StoredMessage stored;
        if (parseStored(raw, slot, stored)) {
            _messages.push_back(stored);
            if (stored.sequence >= _nextSequence) _nextSequence = stored.sequence + 1;
        } else {
            _prefs.remove(key);
        }
    }
    sortMessages();
    return true;
}

void MessageStore::clear() {
    _prefs.clear();
    _messages.clear();
    _nextSequence = 1;
    _prefs.putUInt("nextseq", _nextSequence);
}

bool MessageStore::containsId(const String& id) const {
    for (const auto& item : _messages) {
        if (item.message.id == id) return true;
    }
    return false;
}

size_t MessageStore::unreadCount() const {
    size_t result = 0;
    for (const auto& item : _messages) if (item.unread) ++result;
    return result;
}

const StoredMessage* MessageStore::at(size_t index) const {
    return index < _messages.size() ? &_messages[index] : nullptr;
}

size_t MessageStore::firstUnreadIndex() const {
    for (size_t i = 0; i < _messages.size(); ++i) if (_messages[i].unread) return i;
    return _messages.empty() ? 0 : _messages.size() - 1;
}

bool MessageStore::markRead(size_t index) {
    if (index >= _messages.size()) return false;
    if (!_messages[index].unread) return true;
    _messages[index].unread = false;
    return persist(_messages[index]);
}

bool MessageStore::add(const Message& message) {
    if (!message.valid() || containsId(message.id)) return false;

    std::vector<core::SlotMeta> slots(build::kMaxStoredMessages);
    for (const auto& item : _messages) {
        slots[item.slot] = {true, !item.unread, item.sequence};
    }
    const size_t chosen = core::selectReplacementSlot(slots);
    if (chosen >= build::kMaxStoredMessages) return false;

    auto replaced = _messages.end();
    for (auto it = _messages.begin(); it != _messages.end(); ++it) {
        if (it->slot == chosen) {
            replaced = it;
            break;
        }
    }

    StoredMessage stored;
    stored.message = message;
    stored.unread = true;
    stored.sequence = _nextSequence++;
    if (_nextSequence == 0) _nextSequence = 1;
    stored.slot = static_cast<uint8_t>(chosen);

    if (!persist(stored)) return false;
    if (replaced != _messages.end()) _messages.erase(replaced);
    _prefs.putUInt("nextseq", _nextSequence);
    _messages.push_back(stored);
    sortMessages();
    return true;
}

void MessageStore::sortMessages() {
    std::sort(_messages.begin(), _messages.end(), [](const StoredMessage& a, const StoredMessage& b) {
        return a.sequence < b.sequence;
    });
}

String MessageStore::serializeStored(const StoredMessage& stored) const {
    JsonDocument doc;
    doc["seq"] = stored.sequence;
    doc["unread"] = stored.unread;
    doc["v"] = stored.message.version;
    doc["id"] = stored.message.id;
    doc["sid"] = stored.message.senderId;
    doc["sender"] = stored.message.sender;
    doc["ts"] = stored.message.timestamp;
    doc["type"] = stored.message.type;
    doc["text"] = stored.message.text;
    String output;
    serializeJson(doc, output);
    return output;
}

bool MessageStore::parseStored(const String& json, uint8_t slot, StoredMessage& out) const {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;
    StoredMessage parsed;
    parsed.sequence = doc["seq"] | 0U;
    parsed.unread = doc["unread"] | true;
    parsed.slot = slot;
    parsed.message.version = doc["v"] | 0;
    parsed.message.id = String(doc["id"] | "");
    parsed.message.senderId = String(doc["sid"] | "");
    parsed.message.sender = String(doc["sender"] | "");
    parsed.message.timestamp = doc["ts"] | 0U;
    parsed.message.type = String(doc["type"] | "");
    parsed.message.text = String(doc["text"] | "");
    if (parsed.sequence == 0 || !parsed.message.valid()) return false;
    out = parsed;
    return true;
}

bool MessageStore::persist(const StoredMessage& stored) {
    char key[5];
    slotKey(stored.slot, key);
    const String raw = serializeStored(stored);
    return _prefs.putString(key, raw) > 0;
}

}  // namespace friendbox::messaging
