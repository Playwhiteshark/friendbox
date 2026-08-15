#pragma once
#include <Arduino.h>

namespace friendbox::util {
String sha256Hex(const String& input);
bool isSha256Hex(const String& value);
}
