/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

const char HTML_SETTINGS_EDIT[] PROGMEM = R"rawliteral(
%HEAD_TEMPLATE%
<figure class="text-center">
    <h1>Edit Configuration</h1>
</figure>
<form method="POST" action="/settingssave" enctype="multipart/form-data">
    <div class="input-group mb-3">
        <span class="input-group-text w-50" id="devicenamedesc">Device Name</span>
        <input type="text" class="form-control" aria-describedby="devicenamedesc" id="devicename" name="post_deviceName" maxlength="35"
            value="%DEVICE_NAME%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttserverdesc">MQTT Server</span>
        <input type="text" class="form-control" aria-describedby="mqttserverdesc" id="mqttserver" name="post_mqttServer" maxlength="35"
            value="%MQTT_SERVER%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttportdesc">MQTT Port</span>
        <input type="text" class="form-control" aria-describedby="mqttportdesc" id="mqttport" name="post_mqttPort" maxlength="5"
            value="%MQTT_PORT%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttuserdesc">MQTT User</span>
        <input type="text" class="form-control" aria-describedby="mqttuserdesc" id="mqttuser" name="post_mqttUser" maxlength="35"
            value="%MQTT_USER%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttpassworddesc">MQTT Password</span>
        <input type="text" class="form-control" aria-describedby="mqttpassworddesc" id="mqttpassword" maxlength="35"
            name="post_mqttPassword" value="%MQTT_PASS%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqtttopicdesc">MQTT Topic</span>
        <input type="text" class="form-control" aria-describedby="mqtttopicdesc" id="mqtttopic" name="post_mqttTopic" maxlength="35"
            value="%MQTT_TOPIC%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttrefreshdesc">MQTT Refresh (sec)</span>
        <input type="text" class="form-control" aria-describedby="mqttrefreshdesc" id="mqttrefresh" maxlength="5"
            name="post_mqttRefresh" value="%MQTT_REFRESH%">
    </div>
    <div class="input-group mb-3">
        <span class="input-group-text w-50" id="mqttjsondesc">MQTT Json Style</span>
        <div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="mqttjsondesc"
                role="switch" id="mqttjson" name="post_mqttjson" value="true" %MQTT_JSON%>
        </div>
    </div>
    <div class="row gx-0 mb-2" id="esp01_settings" style="display: %ESP01%;">
    <div class="input-group mb-2">
        <span class="input-group-text w-100"><b>BMS-Wakeup Settings</b></span>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="wakeupenabledesc">Enable BMS Wakeup GPIO%WAKEUP_PIN%</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="wakeupenabledesc"
                role="switch" id="wakeupenable" name="post_wakeupenable" value="true" %BMS_WAKE%>
        </div>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-100"><b>Output Settings</b></span>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisenabledesc">Enable Output on GPIO%RELAISPIN%</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="relaisenabledesc"
                role="switch" id="relaisenable" name="post_relaisenable" value="true" %RELAIS_ENABLE%>
        </div>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisinvertdesc">Invert GPIO Output</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="relaisinvertdesc"
                role="switch" id="relaisinvert" name="post_relaisinvert" value="true" %RELAIS_INVERT%>
        </div>
    </div>

    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisfailsafedesc">Failsafe Mode</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="relaisfailsafedesc"
                role="switch" id="relaisfailsafe" name="post_relaisfailsafe" value="true" %RELAIS_FAILSAVE%>
        </div>
    </div>

	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisfunctiondesc">Function</span>
        <select class="form-select" aria-describedby="relaisfunctiondesc" id="relaisfunction" name="post_relaisfunction">
             <option value="0">Lowest Cell Voltage</option>
			 <option value="1">Highest Cell Voltage</option>
			 <option value="2">Pack Cell Voltage</option>
			 <option value="3">Temperature</option>
             <option value="4">Manual over Web or MQTT</option>
		</select>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaiscomparsiondesc">Comparsion</span>
        <select class="form-select" aria-describedby="relaiscomparsiondesc" id="relaiscomparsion" name="post_relaiscomparsion">
             <option value="0">Higher or equal than</option>
			 <option value="1">Lower or equal than</option>
		</select>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaissetvaluedesc">Value</span>
        <input type="number" step="0.001" class="form-control" aria-describedby="relaissetvaluedesc" id="relaissetvalue"
            name="post_relaissetvalue" value="%RELAIS_VALUE%" min="-100" max="100">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaishysteresisdesc">Hysteresis</span>
        <input type="number" step="0.001" class="form-control" aria-describedby="relaishysteresisdesc" id="relaishysteresis"
            name="post_relaishysteresis" value="%RELAIS_HYST%" min="-100" max="100">
    </div>
    </div>
    <div class="d-grid gap-2">
        <input class="btn btn-primary" type="submit" value="Save settings">
        <a class="btn btn-primary" href="/settings" role="button">Back</a>
    </div>
</form>
<script>
    $(document).ready(function (load) {
        $("#relaisfunction").val(%RELAIS_FUNCTION%);
        $("#relaiscomparsion").val(%RELAIS_COMP%);
        if(document.getElementById("relaisfunction").value == 4){
            RelaisFunctionChange();
        }
    });
    document.getElementById('relaisfunction').addEventListener('change', RelaisFunctionChange);
    RelaisFunctionChange();
    function RelaisFunctionChange(){
        console.log("RelaisFunctionChange()");
        if(document.getElementById("relaisfunction").value == 4){
            relaiscomparsion.setAttribute('disabled', 'disabled');
            relaissetvalue.setAttribute('disabled', 'disabled');
            relaishysteresis.setAttribute('disabled', 'disabled');
        } else {
            relaiscomparsion.removeAttribute("disabled");
            relaissetvalue.removeAttribute("disabled");
            relaishysteresis.removeAttribute("disabled");
        }
    }
</script>
%FOOT_TEMPLATE%
)rawliteral";