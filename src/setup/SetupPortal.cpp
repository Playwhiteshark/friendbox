#include "SetupPortal.h"

#include <WiFiManager.h>
#include "FriendBoxCore.h"

namespace friendbox::setup {
namespace {
const char kHead[] = R"HTML(
<style>
body{background:#0c0f12;color:#eef3f6;font-family:system-ui,-apple-system,sans-serif}
.wrap{max-width:430px}button,.btn{border-radius:10px}input{border-radius:8px}
h1,h2,h3{letter-spacing:.04em}.msg{color:#9deff2}
</style>
<script>
document.addEventListener('DOMContentLoaded',function(){
 const z=document.getElementById('tz');
 if(z && z.value.toUpperCase()==='AUTO') z.value=String(-new Date().getTimezoneOffset());
});
</script>
)HTML";

int16_t parseOffset(const String& value, int16_t fallback) {
    if (value.isEmpty()) return fallback;
    const long parsed = value.toInt();
    if (parsed < -840 || parsed > 840) return fallback;
    return static_cast<int16_t>(parsed);
}
}

SetupResult SetupPortal::run(config::DeviceConfig& config, bool forced) {
    auto& current = config.mutableSettings();
    WiFiManager wm;
    wm.setTitle("FriendBox Setup");
    wm.setCustomHeadElement(kHead);
    wm.setConfigPortalBlocking(true);
    wm.setDarkMode(true);
    (void)forced;

    char port[7];
    snprintf(port, sizeof(port), "%u", current.mqttPort);
    char offset[8];
    if (current.complete()) snprintf(offset, sizeof(offset), "%d", current.utcOffsetMinutes);
    else snprintf(offset, sizeof(offset), "AUTO");

    WiFiManagerParameter pName("name", "Your name", current.displayName.c_str(), 25);
    WiFiManagerParameter pGroup("group", "Room code (blank = create)", current.groupCode.c_str(), 7);
    WiFiManagerParameter pGroupPass("gpass", "Room password (blank = create)", current.groupPassword.c_str(), 7, "type='password'");
    WiFiManagerParameter pAdvancedOpen("<details open><summary><b>Service settings</b></summary><p>Required on first setup; normally leave unchanged later.</p>");
    WiFiManagerParameter pHost("mhost", "MQTT host", current.mqttHost.c_str(), 96);
    WiFiManagerParameter pPort("mport", "MQTT TLS port", port, 6);
    WiFiManagerParameter pUser("muser", "MQTT username", current.mqttUsername.c_str(), 64);
    WiFiManagerParameter pPass("mpass", "MQTT password", current.mqttPassword.c_str(), 96, "type='password'");
    WiFiManagerParameter pAdvancedClose("</details>");
    WiFiManagerParameter pAccent("accent", "Accent color", core::accentName(current.accent), 10,
                                 "list='friendbox-accents' autocomplete='off'");
    WiFiManagerParameter pAccentOptions(
        "<datalist id='friendbox-accents'><option value='cyan'><option value='blue'>"
        "<option value='green'><option value='orange'><option value='pink'><option value='purple'></datalist>");
    WiFiManagerParameter pTz("tz", "UTC offset minutes (auto from phone)", offset, 7);

    wm.addParameter(&pName);
    wm.addParameter(&pGroup);
    wm.addParameter(&pGroupPass);
    wm.addParameter(&pAdvancedOpen);
    wm.addParameter(&pHost);
    wm.addParameter(&pPort);
    wm.addParameter(&pUser);
    wm.addParameter(&pPass);
    wm.addParameter(&pAdvancedClose);
    wm.addParameter(&pAccent);
    wm.addParameter(&pAccentOptions);
    wm.addParameter(&pTz);

    const String apName = current.setupApName();
    if (!wm.startConfigPortal(apName.c_str())) return {};

    current.displayName = String(pName.getValue());
    current.displayName.trim();
    current.mqttHost = String(pHost.getValue());
    current.mqttHost.trim();
    current.mqttPort = static_cast<uint16_t>(constrain(String(pPort.getValue()).toInt(), 1L, 65535L));
    current.mqttUsername = String(pUser.getValue());
    current.mqttPassword = String(pPass.getValue());
    current.utcOffsetMinutes = parseOffset(String(pTz.getValue()), current.utcOffsetMinutes);
    current.accent = core::parseAccent(String(pAccent.getValue()).c_str(), current.accent);

    String group = String(pGroup.getValue()); group.trim(); group.toUpperCase();
    String groupPass = String(pGroupPass.getValue()); groupPass.trim();
    SetupResult result;
    if (group.isEmpty() && groupPass.isEmpty()) {
        if (!config.createRoom()) return {};
        result.createdRoom = true;
    } else if (!config.joinRoom(group, groupPass)) {
        return {};
    }

    result.saved = config.save() && current.complete();
    return result;
}

}  // namespace friendbox::setup
