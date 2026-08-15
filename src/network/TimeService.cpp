#include "TimeService.h"
#include <time.h>
#include "BuildConfig.h"

namespace friendbox::network {

void TimeService::begin() {}

void TimeService::update(bool wifiConnected) {
    if (wifiConnected && !_started) {
        configTime(0, 0, build::kDefaultNtp1, build::kDefaultNtp2);
        _started = true;
    }
}

uint32_t TimeService::epoch() const {
    const time_t now = time(nullptr);
    return now > 0 ? static_cast<uint32_t>(now) : 0U;
}

bool TimeService::valid() const { return epoch() > 1700000000U; }

String TimeService::clockText(int16_t utcOffsetMinutes) const {
    if (!valid()) return "--:--";
    time_t adjusted = static_cast<time_t>(epoch()) + static_cast<int32_t>(utcOffsetMinutes) * 60;
    struct tm value{};
    gmtime_r(&adjusted, &value);
    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", &value);
    return String(buffer);
}

String TimeService::messageTime(uint32_t timestamp, int16_t utcOffsetMinutes) const {
    if (timestamp == 0) return "time unavailable";
    time_t adjusted = static_cast<time_t>(timestamp) + static_cast<int32_t>(utcOffsetMinutes) * 60;
    struct tm value{};
    gmtime_r(&adjusted, &value);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &value);
    return String(buffer);
}

}  // namespace friendbox::network
