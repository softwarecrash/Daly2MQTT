/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/
//https://websemantics.uk/tools/image-to-data-uri-converter/
const char HTML_HEAD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" xml:lang="en">
<head>
    <meta http-equiv="content-type" content="text/html;charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="shortcut icon"
        href="data:image/x-icon;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAAHsSURBVDhPrZLLSxtRFMZHY9A2Wou0FB+oiZUWTaPEVo15qInBjamSqLFVk0rV0ke0saAmgg90o5v42NQHCmYkXehC/wbFuK//gG6yyiIki0Dg69zDEEgp46L+4HKHc8/3zTnnXg7/yf0b7O7tIZVK0XcikaBdCjKIx+MI8jzGxj+hWadHu9mC94NDlHAXZBAIrENTVw+O49LrlaYOfv8cJUnBnV9cYNjlJlGOPBfZMnnaZGrqBw6DQTH131AFLtcHEjDxQ0UBunvsMBhMWFhcoiQpyODj6Fj6rz6fH03NOmpL2/BGmEkLnZdXVGJ+YRFPnj5DKPSLxAzu5uYWkUgEXba3KHxchP39AyhVVVheXoFarYHZ0oHPX76iplYNj2cCptY29DsHRLlYQTKZxOnpGaLRKFRV1dAbjLAJhsUlZdBqX8PpfIeS0jKhCiW+e73gsmQkZpABIxaL4ff1NbXxzeNBVnYOFPmPUP3iJazWTuQ9UNCQJyYnKef4+IR0aQMGzx/Bbu/Fz+0dDA4Nw+0ewfTMLFZX1+imWIw9NEdvPzY2t0iTYeAT7r1Fb4TBaBIeUwfNRal6TiU7HH0UY2dt7WY0NulIk2HwN2y4FosV8tw8XIbDYjQTSYPw1RUC6xvU0mGQF6OZSBrcDfAHIwsaPAvZdQgAAAAASUVORK5CYII=">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet"
        integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js"
        integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p"
        crossorigin="anonymous"></script>
    <title>DALY BMS 2 MQTT</title>
</head>
<body>
    <noscript>
        <strong>We're sorry but it doesn't work properly without JavaScript enabled. Please enable it to
            continue.</strong>
    </noscript>
    <div class="container-md col-md-4">
)rawliteral";

const char HTML_FOOT[] PROGMEM = R"rawliteral(
        <figure class="text-center">DALY BMS to MQTT <a id="software_version">%SOFTWARE_VERSION%</a> By <a href="https://github.com/softwarecrash/"
                target="_blank">Softwarecrash</a></figure>
        <figure class="text-center"><a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/" target="_blank"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/80x15.png" /></a></figure>
    </div>
    <div id="update_alert" style="display: none;">
        <figure class="text-center"><a id="fwdownload" target="_blank">Download the latest version <b id="gitversion"></b></a></figure>
    </div>
<script>
    $(document).ready (function () {
        $.getJSON("https://api.github.com/repos/softwarecrash/DALY-BMS-to-MQTT/releases/latest", function() {
            })
            .done (function (data) {
            console.log("get data from github done success");
            $ ('#fwdownload').attr ('href', data.html_url); 
            $ ('#gitversion').text (data.tag_name.substring(1));
            let x=data.tag_name.substring(1).split('.').map(e=> parseInt(e));
            let y="%SWVERSION%".split('.').map(e=> parseInt(e));
            let z = "";
            for(i=0;i<x.length;i++) {if(x[i] === y[i]) {z+="e";} else if(x[i] > y[i]) {z+="m";} else {z+="l";}}
            if (!z.match(/[l|m]/g)) {console.log("Git-Version equal, nothing to do.");
                } else if (z.split('e').join('')[0] == "m") {console.log("Git-Version higher, activate notification.");
                    document.getElementById("update_alert").style.display = '';
                } else {console.log("Git-Version lower, nothing to do.");}
            })
            .fail(function() {
				console.log("error can not get version");
			});
	});
</script>
</body>
</html>
)rawliteral";