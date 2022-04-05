const char HTML_SETTINGS_EDIT[] PROGMEM = R"rawliteral(
<figure class="text-center"><h1>Edit Configuration</h1></figure>
<form method="POST" action="/settingssave" enctype="multipart/form-data">
<div class="input-group mb-3">
<span class="input-group-text w-50" id="devicenamedesc">Device Name</span>
<input type="text" class="form-control" aria-describedby="devicenamedesc" id="devicename" name="post_deviceName" value="">
</div>
<div class="input-group mb-2">
<span class="input-group-text w-50" id="mqttserverdesc">MQTT Server</span>
<input type="text" class="form-control" aria-describedby="mqttserverdesc" id="mqttserver" name="post_mqttServer" value="">
</div>
<div class="input-group mb-2">
<span class="input-group-text w-50" id="mqttportdesc">MQTT Port</span>
<input type="text" class="form-control" aria-describedby="mqttportdesc" id="mqttport" name="post_mqttPort" value="">
</div>
<div class="input-group mb-2">
<span class="input-group-text w-50" id="mqttuserdesc">MQTT User</span>
<input type="text" class="form-control" aria-describedby="mqttuserdesc" id="mqttuser" name="post_mqttUser" value="">
</div>
<div class="input-group mb-2">
<span class="input-group-text w-50" id="mqttpassworddesc">MQTT Password</span>
<input type="text" class="form-control" aria-describedby="mqttpassworddesc" id="mqttpassword" name="post_mqttPassword" value="">
</div>
<div class="input-group mb-2">
<span class="input-group-text w-50" id="mqtttopicdesc">MQTT Topic</span>
<input type="text" class="form-control" aria-describedby="mqtttopicdesc" id="mqtttopic" name="post_mqttTopic" value="">
</div>
<div class="input-group mb-2">
<span class="input-group-text w-50" id="mqttrefreshdesc">MQTT Refresh</span>
<input type="text" class="form-control" aria-describedby="mqttrefreshdesc" id="mqttrefresh" name="post_mqttRefresh" value="">
</div>
<div class="d-grid gap-2">
<input class="btn btn-primary" type="submit" value="Save settings">
</form>
<a class="btn btn-primary" href="/settings" role="button">Back</a>
</div><script>
        $(document).ready(function(load) {
        $.ajax({
            url: "settingsjson",
            data: {},
            type: "get",
            dataType: "json",
               cache: false,
                success: function (data) {
               document.getElementById("devicename").value = data.device_name;
               document.getElementById("mqttserver").value = data.mqtt_server;
               document.getElementById("mqttport").value = data.mqtt_port;
               document.getElementById("mqtttopic").value = data.mqtt_topic;
               document.getElementById("mqttuser").value = data.mqtt_user;
               document.getElementById("mqttpassword").value = data.mqtt_password;
               document.getElementById("mqttrefresh").value = data.mqtt_refresh;
            }
        });
        });
</script>
)rawliteral";