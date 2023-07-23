/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
const char HTML_MAIN[] PROGMEM = R"rawliteral(
    %HEAD_TEMPLATE%
<div class="row gx-0 mb-2" id="vcc_alert" style="display: none;">
    <div class="alert alert-danger" role="alert" style="text-align: center;">
    <span><b>WARNING ESP VOLTAGE TO LOW</b></span>
    </div>
</div>
<figure class="text-center">
    <h2 id="devicename"></h2>
</figure>
<div class="row gx-0 mb-2">
    <div class="col" id="ClickSOC">
        <div class="progress" style="height:1.8rem;">
            <div id="packSOC" class="progress-bar" role="progressbar" style="width:0%;height:1.8rem;" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100"></div>
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
            <button id="wakebms" type="button" class="btn btn-warning" style="padding: 0px;font-size: 12px;">Wake BMS</button>
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
<div class="row gx-0 mb-2" style="%ESP01%">
    <div class="row gx-0 mb-2">
        <div class="col">
            <div class="bg-light">Relais Output: </div>
        </div>
        <div class="col">
            <div class="bg-light form-check form-switch"><input class="form-check-input" type="checkbox" role="switch"
                    id="relaisOutputActive" disabled></div>
        </div>
    </div>
</div>
<div class="d-grid gap-2">
    <a class="btn btn-primary btn-block" href="/settings" role="button">Settings</a>
</div>

<script>
$(document).ready(function () {
        initWebSocket();
        initButton();
        });
    var gateway = `ws://${window.location.host}/ws`;
    var websocket;
    var ctx;
    var cellChart;
    var createBarChart = true;

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

        if(data.Pack.Status == "offline"){
            document.getElementById("status").style.color = "red";
            document.getElementById("wakebms").style.display = '';
        } else {
            document.getElementById("status").style.color = "black";
            document.getElementById("wakebms").style.display = 'none';
        }
        if(data.Device.Relais_Manual){
            relaisOutputActive.removeAttribute("disabled")
        } else{
            relaisOutputActive.setAttribute('disabled', 'disabled');
        }
        if (data.Device.ESP_VCC < 2.6) {
            document.getElementById("vcc_alert").style.display = '';
        }else{
            document.getElementById("vcc_alert").style.display = 'none';
        }   
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

function BarChart(dataObj)
{
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
            cellCount[tmpCountV] = tmpCountV+1;
            if(tmpCountV == dataObj.Pack.High_CellNr-1){cellColor[tmpCountV] = 'DarkBlue';}
            else if(tmpCountV == dataObj.Pack.Low_CellNr-1){cellColor[tmpCountV] = 'LightSkyBlue';}
            else {cellColor[tmpCountV] = '#0a58ca';}
            if(tmpCountV == dataObj.Pack.High_CellNr-1){cellColor[tmpCountV] = 'DarkBlue';}
            tmpCountV = tmpCountV+1;
        } else {
            cellBalances.push(tmpCellV[i]);
            if(tmpCellV[i] == true){cellColor[tmpCountB] = 'BlueViolet';}
            tmpCountB = tmpCountB+1;
        }
        
    }    
    if(createBarChart == true)
    {
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
            plugins:{
                legend: {display: false},
                title: {display: false},
                label: {display: false}
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
}else{
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
%FOOT_TEMPLATE%
)rawliteral";