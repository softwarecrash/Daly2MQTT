#pragma once

#include <Arduino.h>  // PROGMEM

const char HTML_CONFIRM_RESET[] PROGMEM = R"rawliteral(%pre_head_template%

<figure class="text-center"><h1>Erase all Data?</h1></figure>
<div class="d-grid gap-2">
<a class="btn btn-danger" href="/reset" role="button">Yes</a>
<a class="btn btn-primary" href="/settings" role="button">No</a>
</div>

%FOOT_TEMPLATE%
<p hidden>Hidden Helper</p> )rawliteral";
const char HTML_FOOT[] PROGMEM = R"rawliteral(</br>
<figure class="text-center">
    Daly2MQTT <a id="software_version">%pre_software_version%</a> By <a href="https://github.com/softwarecrash/Daly2MQTT"
        target="_blank">Softwarecrash</a>
    <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/" target="_blank"><img
            alt="Creative Commons License" style="border-width:0"
            src="https://licensebuttons.net/l/by-nc-sa/4.0/80x15.png" /></a>
</figure>
</div>
<div id="update_alert" style="display: none;">
    <figure class="text-center"><a id="fwdownload" target="_blank">Download the latest version <b
                id="gitversion"></b></a></figure>
</div>

<script>
    $(document).ready(function () {
        $.getJSON("https://api.github.com/repos/softwarecrash/Daly2MQTT/releases/latest", function () {
        })
            .done(function (data) {
                console.log("get data from github done success");
                $('#fwdownload').attr('href', data.html_url);
                $('#gitversion').text(data.tag_name.substring(1));
                let x = data.tag_name.substring(1).split('.').map(e => parseInt(e));
                let y = "%pre_swversion%".split('.').map(e => parseInt(e));
                let z = "";
                for (i = 0; i < x.length; i++) { if (x[i] === y[i]) { z += "e"; } else if (x[i] > y[i]) { z += "m"; } else { z += "l"; } }
                if (!z.match(/[l|m]/g)) {
                    console.log("Git-Version equal, nothing to do.");
                } else if (z.split('e').join('')[0] == "m") {
                    console.log("Git-Version higher, activate notification.");
                    document.getElementById("update_alert").style.display = '';
                } else { console.log("Git-Version lower, nothing to do."); }
            })
            .fail(function () {
                console.log("error can not get version");
            });
    });
</script>

</body>

</html>)rawliteral";
const char HTML_HEAD[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en" xml:lang="en">

<head>
    <meta http-equiv="content-type" content="text/html;charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="shortcut icon"
        href="data:image/x-icon;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAAHsSURBVDhPrZLLSxtRFMZHY9A2Wou0FB+oiZUWTaPEVo15qInBjamSqLFVk0rV0ke0saAmgg90o5v42NQHCmYkXehC/wbFuK//gG6yyiIki0Dg69zDEEgp46L+4HKHc8/3zTnnXg7/yf0b7O7tIZVK0XcikaBdCjKIx+MI8jzGxj+hWadHu9mC94NDlHAXZBAIrENTVw+O49LrlaYOfv8cJUnBnV9cYNjlJlGOPBfZMnnaZGrqBw6DQTH131AFLtcHEjDxQ0UBunvsMBhMWFhcoiQpyODj6Fj6rz6fH03NOmpL2/BGmEkLnZdXVGJ+YRFPnj5DKPSLxAzu5uYWkUgEXba3KHxchP39AyhVVVheXoFarYHZ0oHPX76iplYNj2cCptY29DsHRLlYQTKZxOnpGaLRKFRV1dAbjLAJhsUlZdBqX8PpfIeS0jKhCiW+e73gsmQkZpABIxaL4ff1NbXxzeNBVnYOFPmPUP3iJazWTuQ9UNCQJyYnKef4+IR0aQMGzx/Bbu/Fz+0dDA4Nw+0ewfTMLFZX1+imWIw9NEdvPzY2t0iTYeAT7r1Fb4TBaBIeUwfNRal6TiU7HH0UY2dt7WY0NulIk2HwN2y4FosV8tw8XIbDYjQTSYPw1RUC6xvU0mGQF6OZSBrcDfAHIwsaPAvZdQgAAAAASUVORK5CYII=">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet"
        integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.3.0/font/bootstrap-icons.css" rel="stylesheet">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js"
        integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p"
        crossorigin="anonymous"></script>

    <script src=" https://cdn.jsdelivr.net/npm/chart.js@4.3.0/dist/chart.umd.min.js "></script>

    <title>DALY BMS 2 MQTT</title>
</head>

<body>
    <noscript>
        <strong>We're sorry but it doesn't work properly without JavaScript enabled. Please enable it to
            continue.</strong>
    </noscript>
    <div class="container-md col-md-4">)rawliteral";
const char HTML_MAIN[] PROGMEM = R"rawliteral(%pre_head_template%

<figure class="text-center">
    <h2 id="devicename"></h2>
</figure>

<div class="row gx-0 mb-1" id="alert" style="padding-bottom: 0rem;padding-top: 0px; display: none;">
    <div id="alert" class="" role="alert" style="text-align: center;">
        <i id="alert_icon" class=""></i>
        <span id="alert_message"></span>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col" id="ClickSOC">
        <div class="progress" style="height:1.8rem;">
            <div id="packSOC" class="progress-bar" role="progressbar" style="width:0%;height:1.8rem;" aria-valuenow="0"
                aria-valuemin="0" aria-valuemax="100"></div>
        </div>
    </div>
</div>

<div id="cellRow" class="row gx-0 mb-2" style="display: none;">
    <div class="col card chart-container" style="position: relative; height:20vh; width:80vw">
        <canvas id="chart"></canvas>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Package:</div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="packV"></span><span id="packA"></span><span id="packP"></span></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Remaining Capacity: </div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="packRes"></span></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Charge Cycles: </div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="packCycles"></span></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Temperature: </div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="packTemp"></span></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Cell Difference: </div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="cellDiff"></span></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Cells ↑/↓:</div>
    </div>
    <div class="col">
        <div class="bg-light"><span id="cellH"></span><span id="cellL"></span></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">BMS Status: </div>
    </div>
    <div class="col">

        <div class="bg-light">
            <span id="status"></span>
            <button id="wakebms" type="button" class="btn btn-warning" style="padding: 0px;font-size: 12px;">Wake
                BMS</button>
        </div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Charge MOS:</div>
    </div>
    <div class="col">
        <div class="bg-light form-check form-switch">
            <input class="form-check-input" type="checkbox" role="switch" id="chargeFetState">
        </div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Discharge MOS:</div>
    </div>
    <div class="col">
        <div class="bg-light form-check form-switch"><input class="form-check-input" type="checkbox" role="switch"
                id="disChargeFetState"></div>
    </div>
</div>

<div class="row gx-0 mb-2">
    <div class="col">
        <div class="bg-light">Balance State: </div>
    </div>
    <div class="col">
        <div class="bg-light form-check form-switch"><input class="form-check-input" type="checkbox" role="switch"
                id="cellBalanceActive" disabled></div>
    </div>
</div>

<div class="row gx-0 mb-2" style="%pre_esp01%">
    <div class="col">
        <div class="bg-light">Relais Output: </div>
    </div>
    <div class="col">
        <div class="bg-light form-check form-switch"><input class="form-check-input" type="checkbox" role="switch"
                id="relaisOutputActive" disabled></div>
    </div>
</div>

<div class="d-grid gap-2">
    <a class="btn btn-primary btn-block" href="/settings" role="button">Settings</a>
</div>


<script>
    $(document).ready(function () {
        initWebSocket();
        initButton();
        setInterval(refreshAlert, 5000);
    });
    var gateway = `ws://${window.location.host}/ws`;
    var websocket;
    var ctx;
    var cellChart;
    var createBarChart = true;
    var alertListArr = [];
    var alertListitem = 0;
    var kickRefresh = true;

    function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onerror = onError;
        websocket.onmessage = onMessage;
    }
    function onOpen(event) {
        console.log('Connection opened');
    }
    function onClose(event) {
        document.getElementById("status").innerHTML = 'WS Closed';
        console.log('Connection closed');
        setTimeout(initWebSocket, 3000);
    }
    function onError(event) {
        document.getElementById("status").innerHTML = 'WS Lost';
        console.log('Connection lost');
    }
    function onMessage(event) {
        var data = JSON.parse(event.data);
        document.getElementById("devicename").innerHTML = data.Device.Name;
        document.getElementById("packV").innerHTML = data.Pack.Voltage + 'V ';
        document.getElementById("packA").innerHTML = data.Pack.Current + 'A  ';
        document.getElementById("packP").innerHTML = Math.round(data.Pack.Power) + 'W  ';
        document.getElementById("packSOC").innerHTML = data.Pack.SOC + '%%';
        $('#packSOC').width(data.Pack.SOC + "%").attr('aria-valuenow', data.Pack.SOC);
        document.getElementById("packRes").innerHTML = data.Pack.Remaining_mAh + 'mAh ';

        document.getElementById("cellH").innerHTML = data.Pack.High_CellNr + '↑' + data.Pack.High_CellV + 'V ';
        document.getElementById("cellL").innerHTML = data.Pack.Low_CellNr + '↓' + data.Pack.Low_CellV + 'V ';

        document.getElementById("packCycles").innerHTML = data.Pack.Cycles + ' ';
        document.getElementById("packTemp").innerHTML = data.Pack.BMS_Temp + '°C ';
        document.getElementById("cellDiff").innerHTML = data.Pack.Cell_Diff + 'mV ';
        document.getElementById("status").innerHTML = data.Pack.Status;
        document.getElementById("chargeFetState").checked = data.Pack.ChargeFET;
        document.getElementById("disChargeFetState").checked = data.Pack.DischargeFET;
        document.getElementById("cellBalanceActive").checked = data.Pack.Balance_Active;
        document.getElementById("relaisOutputActive").checked = data.Device.Relais_Active;

        BarChart(data);
        alert(data);

        if (data.Pack.Status == "offline") {
            document.getElementById("status").style.color = "red";
            document.getElementById("wakebms").style.display = '';
        } else {
            document.getElementById("status").style.color = "black";
            document.getElementById("wakebms").style.display = 'none';
        }
        if (data.Device.Relais_Manual) {
            relaisOutputActive.removeAttribute("disabled")
        } else {
            relaisOutputActive.setAttribute('disabled', 'disabled');
        }
    }

    function alert(data) {
        alertListArr = [];
        if (data.Device.ESP_VCC < 2.6) {
            alertListArr.push("ESP Volt to Low");
        }
        if (!data.Pack.Fail_Codes.length == 0) {
            var i;
            var i_list = data.Pack.Fail_Codes.split(',');
            for (var i_index in i_list) {
                i = i_list[i_index];
                alertListArr.push(i_list[i_index]);
            }
        }
        if (alertListArr.length == 0) {
            document.getElementById("alert").style.display = 'none';
        } else {
            document.getElementById("alert").style.display = '';
        }
        if (kickRefresh) {
            refreshAlert();
            kickRefresh = false;
        }
    }

    function refreshAlert() {
        var alertValue;
        if (alertListitem < alertListArr.length - 1) {
            alertValue = (alertListArr[alertListitem]);
            alertListitem++;
        } else {
            alertValue = (alertListArr[alertListitem]);
            alertListitem = 0;
        }
        if (typeof alertValue !== 'undefined') {
            if (alertValue[alertValue.length - 1] == "1") {

                document.getElementById("alert_icon").className = "bi bi-info-circle-fill";
                document.getElementById("alert").className = "row gx-0 mb-2 alert alert-info";
            } else if (alertValue[alertValue.length - 1] == "2") {
                document.getElementById("alert_icon").className = "bi bi-exclamation-circle-fill";
                document.getElementById("alert").className = "row gx-0 mb-2 alert alert-warning";
            } else {
                document.getElementById("alert_icon").className = "bi bi-x-circle-fill";
                document.getElementById("alert").className = "row gx-0 mb-2 alert alert-danger";
            }
        }
        document.getElementById('alert_message').innerHTML = (alertValue);
    }

    function initButton() {
        document.getElementById('chargeFetState').addEventListener('click', ChargeFetSwitch);
        document.getElementById('disChargeFetState').addEventListener('click', DischargeFetSwitch);
        document.getElementById('relaisOutputActive').addEventListener('click', RelaisOutputSwitch);
        document.getElementById('wakebms').addEventListener('click', wakeBms);
        document.getElementById('ClickSOC').addEventListener('click', cellState);
    }

    function wakeBms() {
        let switchVal;
        switchVal = 'wake_bms';
        websocket.send(switchVal);
    }

    function cellState() {
        var x = document.getElementById("cellRow");
        if (x.style.display === "none") {
            x.style.display = "";
        } else {
            x.style.display = "none";
        }
    }

    function ChargeFetSwitch() {
        let switchVal;
        if (document.getElementById('chargeFetState').checked) { switchVal = 'chargeFetSwitch_on' }
        else { switchVal = 'chargeFetSwitch_off' }
        websocket.send(switchVal);
    }

    function RelaisOutputSwitch() {
        let switchVal;
        if (document.getElementById('relaisOutputActive').checked) { switchVal = 'relaisOutputSwitch_on' }
        else { switchVal = 'relaisOutputSwitch_off' }
        websocket.send(switchVal);
    }

    function DischargeFetSwitch() {
        let switchVal;
        if (document.getElementById('disChargeFetState').checked) {
            switchVal = 'dischargeFetSwitch_on';
            websocket.send(switchVal);
        }
        else {
            switchVal = 'dischargeFetSwitch_off';
            var check = confirm('Are you sure to disable the DISCHARGE MOS?! You maybe create your own personal blackout!');
            if (check) {
                websocket.send(switchVal);
            } else {
                document.getElementById("disChargeFetState").checked = true;
            }
        }
    }

    function BarChart(dataObj) {
        var tmpCellV = Object.values(dataObj.CellV);
        var cellVoltages = [];
        var cellBalances = [];
        var cellCount = [];
        var cellColor = [];
        var tmpCountV = 0;
        var tmpCountB = 0;
        for (let i = 0; i < tmpCellV.length; i++) {
            if (i % 2 == 0) {
                cellVoltages.push(tmpCellV[i]);
                cellCount[tmpCountV] = tmpCountV + 1;
                if (tmpCountV == dataObj.Pack.High_CellNr - 1) { cellColor[tmpCountV] = 'DarkBlue'; }
                else if (tmpCountV == dataObj.Pack.Low_CellNr - 1) { cellColor[tmpCountV] = 'LightSkyBlue'; }
                else { cellColor[tmpCountV] = '#0a58ca'; }
                if (tmpCountV == dataObj.Pack.High_CellNr - 1) { cellColor[tmpCountV] = 'DarkBlue'; }
                tmpCountV = tmpCountV + 1;
            } else {
                cellBalances.push(tmpCellV[i]);
                if (tmpCellV[i] == true) { cellColor[tmpCountB] = 'BlueViolet'; }
                tmpCountB = tmpCountB + 1;
            }
        }
        if (createBarChart == true) {
            createBarChart = false;
            ctx = document.getElementById("chart").getContext('2d');
            cellChart = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: cellCount,
                    datasets: [{
                        label: 'Cell Voltage',
                        backgroundColor: cellColor,
                        borderColor: 'rgb(13, 110, 253)',
                        data: cellVoltages,
                    }]
                },
                options: {
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: false },
                        title: { display: false },
                        label: { display: false }
                    },
                    scales: {
                        y: {
                            min: dataObj.Pack.cell_lVt,
                            max: dataObj.Pack.cell_hVt
                        },
                        x: {
                            display: true
                        }
                    }
                },
            });
        } else {
            cellChart.data.datasets.pop();
            cellChart.data.datasets.push({
                backgroundColor: cellColor,
                borderColor: 'rgb(10, 88, 202)',
                labels: cellCount,
                label: 'Cell Voltage',
                data: cellVoltages
            });
            cellChart.update('none');
        }
    }
</script>

%pre_foot_template%
<p hidden>Hidden Helper</p> )rawliteral";
const char HTML_REBOOT[] PROGMEM = R"rawliteral(%pre_head_template%

<figure class="text-center">
  <h1>Rebooting </h1>
  <h2 id="wait">.</h2>
</figure>
<div class="d-grid gap-2">
  <a class="btn btn-primary" href="/" role="button">Main</a>
</div>

<script>
  $(document).ready(function () {
    window.dotsGoingUp = true;
    var dots = window.setInterval(function () {
      var wait = document.getElementById("wait");
      if (window.dotsGoingUp)
        wait.innerHTML += ".";
      else {
        wait.innerHTML = wait.innerHTML.substring(1, wait.innerHTML.length);
        if (wait.innerHTML === ".")
          window.dotsGoingUp = true;
      }
      if (wait.innerHTML.length > 9)
        window.dotsGoingUp = false;
    }, 100);

    setTimeout(startCheck, 2000);

    function startCheck() {
      setInterval(serverReachable, 1000);
    }

    function serverReachable() {
      $.get("/").done(function () {
        window.location = "/";
        console.log("success");
      }).fail(function () {
        console.log("failed.");
      });
    }

  });
</script>

%pre_foot_template%
<p hidden>Hidden Helper</p>)rawliteral";
const char HTML_SETTINGS[] PROGMEM = R"rawliteral(%pre_head_template%

<figure class="text-center">
    <h1>Settings</h1>
</figure>
<div class="d-grid gap-2">
    <form method="POST" action="/update" enctype="multipart/form-data">
        <div class="input-group" id="flash_block" style="display: none;">
            <input class="form-control" id="inputGroupFile04" aria-describedby="inputGroupFileAddon04"
                aria-label="Upload" type="file" name="update"><input class="btn btn-outline-secondary"
                id="inputGroupFileAddon04" type="submit" value="Update">
        </div>
    </form>
    <div class="row gx-0 mb-2" id="flash_alert" style="display: none;">
        <div class="alert alert-warning" style="text-align: center;">
            <span><b>ESP FLASH TO SMALL FOR OTA-UPDATE</b></span>
        </div>
    </div>
    <a class="btn btn-primary" href="/settingsedit" role="button">Configure</a>
    <a class="btn btn-warning" href="/reboot" role="button">Reboot</a>
    <a class="btn btn-primary" href="/confirmreset" role="button">Reset ESP</a>
    <a class="btn btn-primary" href="/" role="button">Back</a>
</div>

<script>
    var Flash_Size = Number("%pre_flash_size%");
    console.log(Flash_Size);
    if (Flash_Size < 1048575) {
        document.getElementById("flash_alert").style.display = '';
        document.getElementById("flash_block").style.display = 'none';
    } else {
        document.getElementById("flash_alert").style.display = 'none';
        document.getElementById("flash_block").style.display = '';
    }

</script>

%pre_foot_template%
<p hidden>Hidden Helper</p> )rawliteral";
const char HTML_SETTINGS_EDIT[] PROGMEM = R"rawliteral(%pre_head_template%

<figure class="text-center">
    <h1>Edit Configuration</h1>
</figure>
<form method="POST" action="/settingssave" enctype="multipart/form-data">
    <div class="input-group mb-3">
        <span class="input-group-text w-50" id="devicenamedesc">Device Name</span>
        <input type="text" class="form-control" aria-describedby="devicenamedesc" id="devicename" name="post_deviceName" maxlength="35"
            value="%pre_device_name%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttserverdesc">MQTT Server</span>
        <input type="text" class="form-control" aria-describedby="mqttserverdesc" id="mqttserver" name="post_mqttServer" maxlength="35"
            value="%pre_mqtt_server%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttportdesc">MQTT Port</span>
        <input type="text" class="form-control" aria-describedby="mqttportdesc" id="mqttport" name="post_mqttPort" maxlength="5"
            value="%pre_mqtt_port%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttuserdesc">MQTT User</span>
        <input type="text" class="form-control" aria-describedby="mqttuserdesc" id="mqttuser" name="post_mqttUser" maxlength="35"
            value="%pre_mqtt_user%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttpassworddesc">MQTT Password</span>
        <input type="password" class="form-control" aria-describedby="mqttpassworddesc" id="mqttpassword" maxlength="35"
            name="post_mqttPassword" value="%pre_mqtt_pass%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqtttopicdesc">MQTT Topic</span>
        <input type="text" class="form-control" aria-describedby="mqtttopicdesc" id="mqtttopic" name="post_mqttTopic" maxlength="35"
            value="%pre_mqtt_topic%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqttrefreshdesc">MQTT Refresh (sec)</span>
        <input type="text" class="form-control" aria-describedby="mqttrefreshdesc" id="mqttrefresh" maxlength="5"
            name="post_mqttRefresh" value="%pre_mqtt_refresh%">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="mqtttriggerdesc">MQTT Data Trigger Path</span>
        <input type="text" class="form-control" aria-describedby="mqtttrigerdesc" id="mqtttrigger" maxlength="80"
            name="post_mqtttrigger" value="%pre_mqtt_mqtttrigger%">
    </div>
    <div class="input-group mb-3">
        <span class="input-group-text w-50" id="mqttjsondesc">MQTT Json Style</span>
        <div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="mqttjsondesc"
                role="switch" id="mqttjson" name="post_mqttjson" value="true" %pre_mqtt_json%>
        </div>
    </div>
    <div class="row gx-0 mb-2" id="esp01_settings" style="%pre_esp01%">
    <div class="input-group mb-2">
        <span class="input-group-text w-100"><b>BMS-Wakeup Settings</b></span>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="wakeupenabledesc">Enable BMS Wakeup GPIO%pre_wakeup_pin%</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="wakeupenabledesc"
                role="switch" id="wakeupenable" name="post_wakeupenable" value="true" %bms_wake%>
        </div>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-100"><b>Output Settings</b></span>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisenabledesc">Enable Output on GPIO%pre_relaispin%</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="relaisenabledesc"
                role="switch" id="relaisenable" name="post_relaisenable" value="true" %pre_relais_enable%>
        </div>
    </div>
	<div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisinvertdesc">Invert GPIO Output</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="relaisinvertdesc"
                role="switch" id="relaisinvert" name="post_relaisinvert" value="true" %pre_relais_invert%>
        </div>
    </div>

    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaisfailsafedesc">Failsafe Mode</span>
		<div class="form-switch form-control mqtt-settings-switch" style="width:50%%; text-align: center;">
            <input type="checkbox" class="form-check-input form control" aria-describedby="relaisfailsafedesc"
                role="switch" id="relaisfailsafe" name="post_relaisfailsafe" value="true" %pre_relais_failsave%>
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
            name="post_relaissetvalue" value="%pre_relais_value%" min="-100" max="100">
    </div>
    <div class="input-group mb-2">
        <span class="input-group-text w-50" id="relaishysteresisdesc">Hysteresis</span>
        <input type="number" step="0.001" class="form-control" aria-describedby="relaishysteresisdesc" id="relaishysteresis"
            name="post_relaishysteresis" value="%pre_relais_hyst%" min="-100" max="100">
    </div>
    </div>
    <div class="d-grid gap-2">
        <input class="btn btn-primary" type="submit" value="Save settings">
        <a class="btn btn-primary" href="/settings" role="button">Back</a>
    </div>
</form>
<script>
    $(document).ready(function (load) {
        $("#relaisfunction").val("%pre_relais_function%");
        $("#relaiscomparsion").val("%pre_relais_comp%");
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

%pre_foot_template%
<p hidden>Hidden Helper</p> )rawliteral";
