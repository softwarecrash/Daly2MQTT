const char HTML_MAIN[] PROGMEM = R"rawliteral(
<figure class="text-center"><h2 id="devicename"></h2></figure>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Package: </div>
</div>
<div class="col">
<div class="bg-light"><span id="packV" >N/A</span><span id="packA" >N/A</span><span id="packSOC" >N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Remaining Capacity: </div>
</div>
<div class="col">
<div class="bg-light"><span id="packRes">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Charge Cycles: </div>
</div>
<div class="col">
<div class="bg-light"><span id="packCycles">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Temperature: </div>
</div>
<div class="col">
<div class="bg-light"><span id="packTemp">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Cells Hi/Lo/Diff: </div>
</div>
<div class="col">
<div class="bg-light"><span id="cellH">N/A</span><span id="cellL">N/A</span><span id="cellDiff">N/A</span></div>
</div>
</div>

<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Charge MOS State: </div>
</div>
<div class="col">
<div class="bg-light form-check form-switch"><!--<span id="chargeFetState1">N/A</span>--><input class="form-check-input" type="checkbox" onchange="togglechargefet(this)" role="switch" id="chargeFetState" /></div>
</div>
</div>

<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Discharge MOS State: </div>
</div>
<div class="col">
<div class="bg-light form-check form-switch"><input class="form-check-input" type="checkbox" onchange="toggledischargefet(this)" role="switch" id="disChargeFetState" /></div>
</div>
</div>

<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Balance State: </div>
</div>
<div class="col">
<div class="bg-light form-check form-switch"><input class="form-check-input" type="checkbox" role="switch" id="cellBalanceActive" disabled/></div>
</div>
</div>

<div class="d-grid gap-2">
<a class="btn btn-primary btn-block" href="/settings" role="button">Settings</a>
</div>
<script>
        $(document).ready(function(load) {
         function fetch() {
        $.ajax({
            url: "livejson",
            data: {},
            type: "get",
            dataType: "json",
               cache: false,
                success: function (data) {
               document.getElementById("packV").innerHTML = data.packV+'V ';
               document.getElementById("packA").innerHTML = data.packA+'A  ';
               document.getElementById("packSOC").innerHTML = data.packSOC+'%%';
               document.getElementById("packRes").innerHTML = data.packRes+'mAh ';
               document.getElementById("packCycles").innerHTML = data.packCycles+' ';
               document.getElementById("packTemp").innerHTML = data.packTemp+'°C ';
               document.getElementById("cellH").innerHTML = data.cellH+'V ';
               document.getElementById("cellL").innerHTML = data.cellL+'V ';
               document.getElementById("cellDiff").innerHTML = data.cellDiff+'mV ';
               document.getElementById("chargeFetState").checked = data.chFet;
               document.getElementById("disChargeFetState").checked = data.disChFet;
               document.getElementById("cellBalanceActive").checked = data.cellBal;
               document.getElementById("devicename").innerHTML = 'Device: '+data.device_name;
            }
        });
        }
        setInterval(fetch, 5000);
        fetch();
        });

function toggledischargefet(element) {
var xhr = new XMLHttpRequest();
if(element.checked){ xhr.open("GET", "/set?dischargefet=1", true); }
else { xhr.open("GET", "/set?dischargefet=0", true); }
xhr.send();
clearInterval();
}
function togglechargefet(element) {
var xhr = new XMLHttpRequest();
if(element.checked){ xhr.open("GET", "/set?chargefet=1", true); }
else { xhr.open("GET", "/set?chargefet=0", true); }
xhr.send();
clearInterval();
}
</script>
)rawliteral";