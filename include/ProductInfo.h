#pragma once

namespace friendbox::product {

// User-facing identity lives here so a visual rename does not accidentally
// change MQTT topics or the room-token derivation protocol.
inline constexpr const char* kName = "FriendBox";
inline constexpr const char* kDisplayTitle = "FRIENDBOX";
inline constexpr const char* kSetupTitle = "FriendBox Setup";
inline constexpr const char* kSetupApPrefix = "FriendBox-Setup-";
inline constexpr const char* kHttpUserAgent = "FriendBox/1";

}  // namespace friendbox::product
