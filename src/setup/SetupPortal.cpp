#include "SetupPortal.h"

#include <WiFiManager.h>
#include <cctype>
#include <vector>
#include "FriendBoxCore.h"
#include "ProductInfo.h"

namespace friendbox::setup {
namespace {

const char kHead[] = R"HTML(
<style>
body{background:#0c0f12;color:#eef3f6;font-family:system-ui,-apple-system,sans-serif}
.wrap{max-width:430px}button,.btn,input,select{border-radius:9px}
h1,h2,h3{letter-spacing:.03em}.msg{color:#9deff2}
.fb-data,label[for=name],label[for=group],label[for=gpass],label[for=mhost],
label[for=mport],label[for=muser],label[for=mpass],label[for=accent],
label[for=color1],label[for=color2],label[for=bright],label[for=timeout],
label[for=clock],label[for=tz],label[for=mdot],label[for=mletter],
label[for=mword],label[for=mcontrol],label[for=p0],label[for=p1],
label[for=p2],label[for=p3],label[for=p4]{display:none!important}
.fb-settings{margin-top:.25rem}.fb-section{margin:1rem 0;padding:1rem;border:1px solid #33404a;border-radius:13px}
.fb-section h2{margin:.1rem 0 .75rem;font-size:1.05rem;color:#9deff2}
.fb-help{font-size:.82rem;color:#b7c1c8;margin:.25rem 0 .8rem}
.fb-field{display:block;margin:.65rem 0}.fb-field span{display:block;margin-bottom:.3rem;font-size:.9rem}
.fb-field input,.fb-field select{box-sizing:border-box;width:100%;padding:.65rem;background:#171c21;color:#eef3f6;border:1px solid #485863}
.fb-color-row{display:grid;grid-template-columns:1fr 72px;gap:.7rem;align-items:end}
.fb-color-row input[type=color]{height:44px;padding:.25rem}
.fb-range{display:grid;grid-template-columns:1fr 3rem;gap:.65rem;align-items:center}
.fb-range output{text-align:center;color:#9deff2}
details.fb-section summary{cursor:pointer;color:#9deff2;font-weight:650}
</style>
<script>
document.addEventListener('DOMContentLoaded',function(){
 const tz=document.getElementById('fb-tz');
 if(tz&&tz.value.toUpperCase()==='AUTO')tz.value=String(-new Date().getTimezoneOffset());
 document.querySelectorAll('[data-target]').forEach(function(el){
   const target=document.getElementById(el.dataset.target);if(!target)return;
   const sync=function(){target.value=el.value;const out=document.getElementById(el.id+'-out');if(out)out.textContent=el.value+'%';};
   el.addEventListener('input',sync);el.addEventListener('change',sync);sync();
 });
});
</script>
)HTML";

String htmlEscape(String value) {
    value.replace("&", "&amp;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");
    value.replace("\"", "&quot;");
    value.replace("'", "&#39;");
    return value;
}

String selected(bool value) {
    return value ? " selected" : "";
}

String buildSettingsHtml(const config::SettingsDraft& initial, bool configured) {
    const String accent = core::accentName(initial.accent);
    const String color1(core::rgbHex(initial.customColor1).c_str());
    const String color2(core::rgbHex(initial.customColor2).c_str());
    const String timezone = configured ? String(initial.utcOffsetMinutes) : String("AUTO");

    String html;
    html.reserve(8500);
    html += "<div class='fb-settings'><p class='fb-help'>FriendBox settings are separate from Wi-Fi. Save this page, then use Configure Wi-Fi if the network also needs changing.</p>";
    html += "<section class='fb-section'><h2>FriendBox</h2>";
    html += "<label class='fb-field'><span>Your name</span><input id='fb-name' data-target='name' maxlength='24' value='" + htmlEscape(initial.displayName) + "'></label>";
    html += "<label class='fb-field'><span>Room code (leave both blank to create)</span><input id='fb-group' data-target='group' maxlength='6' value='" + htmlEscape(initial.groupCode) + "'></label>";
    html += "<label class='fb-field'><span>Room password</span><input id='fb-gpass' data-target='gpass' type='password' inputmode='numeric' maxlength='6' value='" + htmlEscape(initial.groupPassword) + "'></label></section>";

    html += "<section class='fb-section'><h2>Preset messages</h2><p class='fb-help'>Blank slots are hidden from the Send screen.</p>";
    for (size_t i = 0; i < initial.presets.size(); ++i) {
        html += "<label class='fb-field'><span>Preset " + String(i + 1) + "</span><input id='fb-p" + String(i) + "' data-target='p" + String(i) + "' maxlength='" + String(core::PresetCatalog::kMaxTextLength) + "' value='" + htmlEscape(initial.presets[i]) + "'></label>";
    }
    html += "</section>";

    html += "<section class='fb-section'><h2>Display</h2><label class='fb-field'><span>Accent</span><select id='fb-accent' data-target='accent'>";
    for (uint8_t i = 0; i < static_cast<uint8_t>(core::Accent::Count); ++i) {
        const String name = core::accentName(static_cast<core::Accent>(i));
        String label = name;
        label.replace("custom1", "Custom 1");
        label.replace("custom2", "Custom 2");
        if (i < static_cast<uint8_t>(core::Accent::Custom1)) {
            label.setCharAt(0, static_cast<char>(toupper(static_cast<unsigned char>(label[0]))));
        }
        html += "<option value='" + name + "'" + selected(name == accent) + ">" + label + "</option>";
    }
    html += "</select></label>";
    html += "<div class='fb-color-row'><label class='fb-field'><span>Custom 1</span><input id='fb-color1-text' data-target='color1' maxlength='7' value='" + color1 + "'></label><label class='fb-field'><span>Pick</span><input id='fb-color1' data-target='color1' type='color' value='" + color1 + "'></label></div>";
    html += "<div class='fb-color-row'><label class='fb-field'><span>Custom 2</span><input id='fb-color2-text' data-target='color2' maxlength='7' value='" + color2 + "'></label><label class='fb-field'><span>Pick</span><input id='fb-color2' data-target='color2' type='color' value='" + color2 + "'></label></div>";
    html += "<label class='fb-field'><span>Brightness</span><div class='fb-range'><input id='fb-bright' data-target='bright' type='range' min='10' max='100' step='5' value='" + String(initial.brightnessPercent) + "'><output id='fb-bright-out'></output></div></label>";
    html += "<label class='fb-field'><span>Turn screen off after</span><select id='fb-timeout' data-target='timeout'>";
    const uint16_t timeoutValues[] = {0, 30, 60, 120, 300, 600};
    const char* timeoutLabels[] = {"Never", "30 seconds", "1 minute", "2 minutes", "5 minutes", "10 minutes"};
    for (size_t i = 0; i < 6; ++i) {
        html += "<option value='" + String(timeoutValues[i]) + "'" + selected(initial.screenTimeoutSeconds == timeoutValues[i]) + ">" + timeoutLabels[i] + "</option>";
    }
    html += "</select></label>";
    html += "<label class='fb-field'><span>Show clock</span><select id='fb-clock' data-target='clock'><option value='1'" + selected(initial.clockVisible) + ">Yes</option><option value='0'" + selected(!initial.clockVisible) + ">No</option></select></label></section>";

    html += "<details class='fb-section'><summary>Morse timing</summary><p class='fb-help'>Defaults are recommended until the button feel has been tested.</p>";
    html += "<label class='fb-field'><span>Dash threshold (ms)</span><input id='fb-mdot' data-target='mdot' type='number' min='100' max='1000' value='" + String(initial.morseTiming.dashThresholdMs) + "'></label>";
    html += "<label class='fb-field'><span>Letter pause (ms)</span><input id='fb-mletter' data-target='mletter' type='number' min='250' max='3000' value='" + String(initial.morseTiming.letterGapMs) + "'></label>";
    html += "<label class='fb-field'><span>Word pause (ms)</span><input id='fb-mword' data-target='mword' type='number' min='251' max='6000' value='" + String(initial.morseTiming.wordGapMs) + "'></label>";
    html += "<label class='fb-field'><span>Command hold (ms)</span><input id='fb-mcontrol' data-target='mcontrol' type='number' min='800' max='5000' value='" + String(initial.morseTiming.controlHoldMs) + "'></label></details>";

    html += "<details class='fb-section'><summary>Advanced service settings</summary><p class='fb-help'>Normally leave these unchanged. Leave the MQTT password blank to keep the saved password.</p>";
    html += "<label class='fb-field'><span>MQTT host</span><input id='fb-mhost' data-target='mhost' maxlength='96' value='" + htmlEscape(initial.mqttHost) + "'></label>";
    html += "<label class='fb-field'><span>MQTT TLS port</span><input id='fb-mport' data-target='mport' type='number' min='1' max='65535' value='" + String(initial.mqttPort) + "'></label>";
    html += "<label class='fb-field'><span>MQTT username</span><input id='fb-muser' data-target='muser' maxlength='64' value='" + htmlEscape(initial.mqttUsername) + "'></label>";
    html += "<label class='fb-field'><span>MQTT password</span><input id='fb-mpass' data-target='mpass' type='password' maxlength='96' value=''></label>";
    html += "<label class='fb-field'><span>UTC offset minutes</span><input id='fb-tz' data-target='tz' inputmode='numeric' maxlength='5' value='" + timezone + "'></label></details></div>";
    return html;
}

int16_t parseOffset(const String& value, int16_t fallback) {
    if (value.isEmpty()) return fallback;
    const long parsed = value.toInt();
    return parsed < -840 || parsed > 840 ? fallback : static_cast<int16_t>(parsed);
}

uint16_t parseU16(const String& value, uint16_t fallback, uint16_t minimum, uint16_t maximum) {
    String trimmed = value;
    trimmed.trim();
    if (trimmed.isEmpty()) return fallback;
    const long parsed = trimmed.toInt();
    return parsed < minimum || parsed > maximum ? fallback : static_cast<uint16_t>(parsed);
}

uint32_t parseColor(const String& value, uint32_t fallback) {
    uint32_t parsed = 0;
    return core::parseRgbHex(value.c_str(), parsed) ? parsed : fallback;
}

class PortalForm {
public:
    PortalForm(const config::SettingsDraft& initial, bool configured)
        : _initial(initial), _formHtml(buildSettingsHtml(initial, configured)),
          _portValue(String(initial.mqttPort)),
          _offsetValue(configured ? String(initial.utcOffsetMinutes) : String("AUTO")),
          _accentValue(core::accentName(initial.accent)),
          _color1Value(core::rgbHex(initial.customColor1).c_str()),
          _color2Value(core::rgbHex(initial.customColor2).c_str()),
          _brightnessValue(String(initial.brightnessPercent)),
          _timeoutValue(String(initial.screenTimeoutSeconds)),
          _clockValue(initial.clockVisible ? "1" : "0"),
          _morseDashValue(String(initial.morseTiming.dashThresholdMs)),
          _morseLetterValue(String(initial.morseTiming.letterGapMs)),
          _morseWordValue(String(initial.morseTiming.wordGapMs)),
          _morseControlValue(String(initial.morseTiming.controlHoldMs)),
          _name("name", "", _initial.displayName.c_str(), 24, kHidden),
          _group("group", "", _initial.groupCode.c_str(), 6, kHidden),
          _groupPassword("gpass", "", _initial.groupPassword.c_str(), 6, kHidden),
          _host("mhost", "", _initial.mqttHost.c_str(), 96, kHidden),
          _port("mport", "", _portValue.c_str(), 5, kHidden),
          _username("muser", "", _initial.mqttUsername.c_str(), 64, kHidden),
          _password("mpass", "", "", 96, kHidden),
          _accent("accent", "", _accentValue.c_str(), 8, kHidden),
          _color1("color1", "", _color1Value.c_str(), 7, kHidden),
          _color2("color2", "", _color2Value.c_str(), 7, kHidden),
          _brightness("bright", "", _brightnessValue.c_str(), 3, kHidden),
          _timeout("timeout", "", _timeoutValue.c_str(), 4, kHidden),
          _clock("clock", "", _clockValue.c_str(), 1, kHidden),
          _timezone("tz", "", _offsetValue.c_str(), 5, kHidden),
          _morseDash("mdot", "", _morseDashValue.c_str(), 4, kHidden),
          _morseLetter("mletter", "", _morseLetterValue.c_str(), 4, kHidden),
          _morseWord("mword", "", _morseWordValue.c_str(), 4, kHidden),
          _morseControl("mcontrol", "", _morseControlValue.c_str(), 4, kHidden),
          _preset0("p0", "", _initial.presets[0].c_str(), core::PresetCatalog::kMaxTextLength, kHidden),
          _preset1("p1", "", _initial.presets[1].c_str(), core::PresetCatalog::kMaxTextLength, kHidden),
          _preset2("p2", "", _initial.presets[2].c_str(), core::PresetCatalog::kMaxTextLength, kHidden),
          _preset3("p3", "", _initial.presets[3].c_str(), core::PresetCatalog::kMaxTextLength, kHidden),
          _preset4("p4", "", _initial.presets[4].c_str(), core::PresetCatalog::kMaxTextLength, kHidden),
          _visibleForm(_formHtml.c_str()) {}

    void addTo(WiFiManager& manager) {
        WiFiManagerParameter* fields[] = {
            &_name, &_group, &_groupPassword, &_host, &_port, &_username, &_password,
            &_accent, &_color1, &_color2, &_brightness, &_timeout, &_clock, &_timezone,
            &_morseDash, &_morseLetter, &_morseWord, &_morseControl,
            &_preset0, &_preset1, &_preset2, &_preset3, &_preset4, &_visibleForm,
        };
        for (auto* field : fields) manager.addParameter(field);
    }

    config::SettingsDraft submitted() {
        config::SettingsDraft result = _initial;
        result.displayName = String(_name.getValue());
        result.groupCode = String(_group.getValue());
        result.groupPassword = String(_groupPassword.getValue());
        result.mqttHost = String(_host.getValue());
        result.mqttPort = parseU16(String(_port.getValue()), _initial.mqttPort, 1, 65535);
        result.mqttUsername = String(_username.getValue());
        const String submittedPassword(_password.getValue());
        if (!submittedPassword.isEmpty()) result.mqttPassword = submittedPassword;
        result.utcOffsetMinutes = parseOffset(String(_timezone.getValue()), _initial.utcOffsetMinutes);
        result.accent = core::parseAccent(String(_accent.getValue()).c_str(), _initial.accent);
        result.customColor1 = parseColor(String(_color1.getValue()), _initial.customColor1);
        result.customColor2 = parseColor(String(_color2.getValue()), _initial.customColor2);
        result.brightnessPercent = static_cast<uint8_t>(parseU16(String(_brightness.getValue()), _initial.brightnessPercent, 10, 100));
        result.screenTimeoutSeconds = parseU16(String(_timeout.getValue()), _initial.screenTimeoutSeconds, 0, 3600);
        result.clockVisible = String(_clock.getValue()) != "0";
        result.morseTiming.dashThresholdMs = parseU16(String(_morseDash.getValue()), _initial.morseTiming.dashThresholdMs, 100, 1000);
        result.morseTiming.letterGapMs = parseU16(String(_morseLetter.getValue()), _initial.morseTiming.letterGapMs, 250, 3000);
        result.morseTiming.wordGapMs = parseU16(String(_morseWord.getValue()), _initial.morseTiming.wordGapMs, 251, 6000);
        result.morseTiming.controlHoldMs = parseU16(String(_morseControl.getValue()), _initial.morseTiming.controlHoldMs, 800, 5000);
        result.presets = {String(_preset0.getValue()), String(_preset1.getValue()),
                          String(_preset2.getValue()), String(_preset3.getValue()),
                          String(_preset4.getValue())};
        return result;
    }

private:
    static constexpr const char* kHidden = "type='hidden' class='fb-data'";
    config::SettingsDraft _initial;
    String _formHtml, _portValue, _offsetValue, _accentValue, _color1Value, _color2Value;
    String _brightnessValue, _timeoutValue, _clockValue;
    String _morseDashValue, _morseLetterValue, _morseWordValue, _morseControlValue;
    WiFiManagerParameter _name, _group, _groupPassword, _host, _port, _username, _password;
    WiFiManagerParameter _accent, _color1, _color2, _brightness, _timeout, _clock, _timezone;
    WiFiManagerParameter _morseDash, _morseLetter, _morseWord, _morseControl;
    WiFiManagerParameter _preset0, _preset1, _preset2, _preset3, _preset4, _visibleForm;
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
    manager.setShowInfoErase(false);
    manager.setShowInfoUpdate(false);
    std::vector<const char*> menu{"param", "wifi", "exit"};
    manager.setMenu(menu);

    PortalForm form(config.draft(), config.settings().complete());
    form.addTo(manager);

    SetupResult result;
    manager.setSaveParamsCallback([&]() {
        const config::SettingsDraft submitted = form.submitted();
        const bool createRoom = !result.createdRoom && requestsNewRoom(submitted);
        const config::RoomAction roomAction = result.createdRoom
                                                  ? config::RoomAction::Keep
                                                  : (createRoom ? config::RoomAction::Create
                                                                : config::RoomAction::Join);
        const bool saved = config.apply(submitted, roomAction);
        result.saved = result.saved || saved;
        result.createdRoom = result.createdRoom || (saved && createRoom);
    });

    manager.startConfigPortal(config.settings().setupApName().c_str());
    return result;
}

}  // namespace friendbox::setup
