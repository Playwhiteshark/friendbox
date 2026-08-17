#include "SetupPortal.h"

#include <WiFiManager.h>
#include "FriendBoxCore.h"
#include "ProductInfo.h"

namespace friendbox::setup {
namespace {

const char kHead[] = R"HTML(
<style>
body{background:#0c0f12;color:#eef3f6;font-family:system-ui,-apple-system,sans-serif}
.wrap{max-width:430px}button,.btn{border-radius:10px}input{border-radius:8px}
h1,h2,h3{letter-spacing:.04em}.msg{color:#9deff2}
details{margin:1rem 0}summary{cursor:pointer}
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

uint16_t parsePort(const String& value, uint16_t fallback) {
    String trimmed = value;
    trimmed.trim();
    if (trimmed.isEmpty()) return fallback;
    const long parsed = trimmed.toInt();
    if (parsed < 1 || parsed > 65535) return fallback;
    return static_cast<uint16_t>(parsed);
}

String accentDatalist() {
    String html("<datalist id='friendbox-accents'>");
    for (uint8_t i = 0; i < static_cast<uint8_t>(core::Accent::Count); ++i) {
        html += "<option value='";
        html += core::accentName(static_cast<core::Accent>(i));
        html += "'>";
    }
    html += "</datalist>";
    return html;
}

class PortalForm {
public:
    PortalForm(const config::SettingsDraft& initial, bool configured)
        : _initial(initial),
          _portValue(String(initial.mqttPort)),
          _offsetValue(configured ? String(initial.utcOffsetMinutes) : String("AUTO")),
          _accentOptionsHtml(accentDatalist()),
          _name("name", "Your name", _initial.displayName.c_str(), 25),
          _group("group", "Room code (blank = create)", _initial.groupCode.c_str(), 7),
          _groupPassword("gpass", "Room password (blank = create)",
                         _initial.groupPassword.c_str(), 7, "type='password'"),
          _advancedOpen(
              "<details><summary><b>Advanced service settings</b></summary>"
              "<p>Normally leave these unchanged. Leave MQTT password blank to keep the saved password.</p>"),
          _host("mhost", "MQTT host", _initial.mqttHost.c_str(), 96),
          _port("mport", "MQTT TLS port", _portValue.c_str(), 6),
          _username("muser", "MQTT username", _initial.mqttUsername.c_str(), 64),
          _password("mpass", "MQTT password", "", 96,
                    "type='password' autocomplete='new-password'"),
          _advancedClose("</details>"),
          _accent("accent", "Accent color", core::accentName(_initial.accent), 10,
                  "list='friendbox-accents' autocomplete='off'"),
          _accentOptions(_accentOptionsHtml.c_str()),
          _timezone("tz", "UTC offset minutes (auto from phone)", _offsetValue.c_str(), 7) {}

    void addTo(WiFiManager& manager) {
        manager.addParameter(&_name);
        manager.addParameter(&_group);
        manager.addParameter(&_groupPassword);
        manager.addParameter(&_advancedOpen);
        manager.addParameter(&_host);
        manager.addParameter(&_port);
        manager.addParameter(&_username);
        manager.addParameter(&_password);
        manager.addParameter(&_advancedClose);
        manager.addParameter(&_accent);
        manager.addParameter(&_accentOptions);
        manager.addParameter(&_timezone);
    }

    config::SettingsDraft submitted() {
        config::SettingsDraft result = _initial;
        result.displayName = String(_name.getValue());
        result.groupCode = String(_group.getValue());
        result.groupPassword = String(_groupPassword.getValue());
        result.mqttHost = String(_host.getValue());
        result.mqttPort = parsePort(String(_port.getValue()), _initial.mqttPort);
        result.mqttUsername = String(_username.getValue());

        const String submittedPassword(_password.getValue());
        if (!submittedPassword.isEmpty()) result.mqttPassword = submittedPassword;

        result.utcOffsetMinutes = parseOffset(String(_timezone.getValue()),
                                              _initial.utcOffsetMinutes);
        result.accent = core::parseAccent(String(_accent.getValue()).c_str(),
                                          _initial.accent);
        return result;
    }

private:
    config::SettingsDraft _initial;
    String _portValue;
    String _offsetValue;
    String _accentOptionsHtml;
    WiFiManagerParameter _name;
    WiFiManagerParameter _group;
    WiFiManagerParameter _groupPassword;
    WiFiManagerParameter _advancedOpen;
    WiFiManagerParameter _host;
    WiFiManagerParameter _port;
    WiFiManagerParameter _username;
    WiFiManagerParameter _password;
    WiFiManagerParameter _advancedClose;
    WiFiManagerParameter _accent;
    WiFiManagerParameter _accentOptions;
    WiFiManagerParameter _timezone;
};

bool requestsNewRoom(const config::SettingsDraft& draft) {
    String code = draft.groupCode;
    String password = draft.groupPassword;
    code.trim();
    password.trim();
    return code.isEmpty() && password.isEmpty();
}

}  // namespace

SetupResult SetupPortal::run(config::DeviceConfig& config) {
    WiFiManager manager;
    manager.setTitle(product::kSetupTitle);
    manager.setCustomHeadElement(kHead);
    manager.setConfigPortalBlocking(true);
    manager.setDarkMode(true);

    PortalForm form(config.draft(), config.settings().complete());
    form.addTo(manager);
    if (!manager.startConfigPortal(config.settings().setupApName().c_str())) return {};

    const config::SettingsDraft submitted = form.submitted();
    const bool createRoom = requestsNewRoom(submitted);
    const config::RoomAction roomAction = createRoom ? config::RoomAction::Create
                                                     : config::RoomAction::Join;
    const bool saved = config.apply(submitted, roomAction);
    return {saved, saved && createRoom};
}

}  // namespace friendbox::setup
