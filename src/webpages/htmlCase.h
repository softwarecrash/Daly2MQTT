/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

const char HTML_HEAD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <meta http-equiv="content-type" content="text/html;charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="shortcut icon"
        href="data:image/x-icon;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAAHsSURBVDhPrZLLSxtRFMZHY9A2Wou0FB+oiZUWTaPEVo15qInBjamSqLFVk0rV0ke0saAmgg90o5v42NQHCmYkXehC/wbFuK//gG6yyiIki0Dg69zDEEgp46L+4HKHc8/3zTnnXg7/yf0b7O7tIZVK0XcikaBdCjKIx+MI8jzGxj+hWadHu9mC94NDlHAXZBAIrENTVw+O49LrlaYOfv8cJUnBnV9cYNjlJlGOPBfZMnnaZGrqBw6DQTH131AFLtcHEjDxQ0UBunvsMBhMWFhcoiQpyODj6Fj6rz6fH03NOmpL2/BGmEkLnZdXVGJ+YRFPnj5DKPSLxAzu5uYWkUgEXba3KHxchP39AyhVVVheXoFarYHZ0oHPX76iplYNj2cCptY29DsHRLlYQTKZxOnpGaLRKFRV1dAbjLAJhsUlZdBqX8PpfIeS0jKhCiW+e73gsmQkZpABIxaL4ff1NbXxzeNBVnYOFPmPUP3iJazWTuQ9UNCQJyYnKef4+IR0aQMGzx/Bbu/Fz+0dDA4Nw+0ewfTMLFZX1+imWIw9NEdvPzY2t0iTYeAT7r1Fb4TBaBIeUwfNRal6TiU7HH0UY2dt7WY0NulIk2HwN2y4FosV8tw8XIbDYjQTSYPw1RUC6xvU0mGQF6OZSBrcDfAHIwsaPAvZdQgAAAAASUVORK5CYII=" />
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
        <figure class="text-center">DALY BMS to MQTT V0.4.15-4 By <a href="https://github.com/softwarecrash/"
                target="_blank">Softwarecrash</a></figure>
    </div>
</body>

</html>
)rawliteral";
