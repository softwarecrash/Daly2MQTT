const char HTML_CONFIRM_RESET[] PROGMEM = R"rawliteral(
<figure class="text-center"><h1>Erease all Data?</h1></figure>
<div class="d-grid gap-2">
<a class="btn btn-danger" href="/reset" role="button">Yes</a>
<a class="btn btn-primary" href="/settings" role="button">No</a>
</div>
)rawliteral";


const char HTML_SETTINGS[] PROGMEM = R"rawliteral(
<figure class="text-center"><h1>Settings</h1></figure>
<div class="d-grid gap-2">
<form method="POST" action="/update" enctype="multipart/form-data">
<div class="input-group">
<input class="form-control" id="inputGroupFile04" aria-describedby="inputGroupFileAddon04" aria-label="Upload" type="file" name="update"><input class="btn btn-outline-secondary" id="inputGroupFileAddon04" type="submit" value="Update">
</div>
</form>
<a class="btn btn-primary" href="/settingsedit" role="button">Cofigure</a>
<a class="btn btn-warning" href="/reboot" role="button">Reboot</a>
<a class="btn btn-primary" href="/confirmreset" role="button">Reset ESP</a>
<a class="btn btn-primary" href="/" role="button">Back</a>
</div>
)rawliteral";


