const char HTML_REBOOT[] PROGMEM = R"rawliteral(
    %HEAD_TEMPLATE%
<figure class="text-center"><h1>Rebooting ...</h1></figure>
<div class="d-grid gap-2">
<a class="btn btn-primary" href="/" role="button">Main</a>
</div>

<script>
$(document).ready(function () {
setTimeout(startCheck, 2000);


function startCheck(){
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

%FOOT_TEMPLATE%
)rawliteral";
