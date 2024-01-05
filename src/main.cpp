/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
#include "main.h"

#if __has_include(<DalySerial.h>)
#include <DalySerial.h>
#else
#include <daly.h>
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Settings.h"

#include "html.h"
#include "htmlProzessor.h" // The html Prozessor

WiFiClient client;
Settings _settings;
PubSubClient mqttclient(client);

StaticJsonDocument<JSON_BUFFER> bmsJson;                          // main Json
JsonObject deviceJson = bmsJson.createNestedObject("Device");     // basic device data
JsonObject packJson = bmsJson.createNestedObject("Pack");         // battery package data
JsonObject cellVJson = bmsJson.createNestedObject("CellV");       // nested data for cell voltages
JsonObject cellTempJson = bmsJson.createNestedObject("CellTemp"); // nested data for cell temp

int mqttdebug;

unsigned long mqtttimer = 0;
unsigned long RestartTimer = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;
#ifndef DALY_SERIAL_H
DalyBms bms(MYPORT_RX, MYPORT_TX);
#else
DalySerial bms(MYPORT_RX, MYPORT_TX);
#endif

// https://randomnerdtutorials.com/esp8266-ds18b20-temperature-sensor-web-server-with-arduino-ide/
OneWire oneWire(TEMPSENS_PIN);
DallasTemperature tempSens(&oneWire);

#include "status-LED.h"

// flag for saving data and other things
bool shouldSaveConfig = false;
bool restartNow = false;
bool workerCanRun = true;
bool haDiscTrigger = false;
bool haAutoDiscTrigger = false;
bool dataCollect = false;
bool firstPublish = false;
bool webSocketAction = false;
unsigned long wakeuptimer = 0; // dont run immediately after boot, wait for first intervall
bool wakeupPinActive = false;
unsigned long relaistimer = 0;
float relaisCompareValueTmp = 0;
bool relaisComparsionResult = false;
uint32_t bootcount = 0;
char mqttClientId[80];
uint8_t numOfTempSens;
DeviceAddress tempDeviceAddress;

ADC_MODE(ADC_VCC);

//----------------------------------------------------------------------
void saveConfigCallback()
{

  DEBUG_PRINTLN(F("<SYS >Should save config"));
  shouldSaveConfig = true;
}

void notifyClients()
{
  if (wsClient != nullptr && wsClient->canSend() && !webSocketAction)
  {
    DEBUG_PRINT(F("<WEBS> Data sent to WebSocket... "));
    DEBUG_WEB(F("<WEBS> Data sent to WebSocket... "));
    size_t len = measureJson(bmsJson);
    AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
    if (buffer)
    {
      serializeJson(bmsJson, (char *)buffer->get(), len + 1);
      wsClient->text(buffer);
    }
    DEBUG_PRINTLN(F("Done"));
    DEBUG_WEBLN(F("Done"));
  }
  webSocketAction = false;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "ping") != 0)
    {
      if (strcmp((char *)data, "dischargeFetSwitch_on") == 0)
      {
        bms.setDischargeMOS(true);
      }
      if (strcmp((char *)data, "dischargeFetSwitch_off") == 0)
      {
        bms.setDischargeMOS(false);
      }
      if (strcmp((char *)data, "chargeFetSwitch_on") == 0)
      {
        bms.setChargeMOS(true);
      }
      if (strcmp((char *)data, "chargeFetSwitch_off") == 0)
      {
        bms.setChargeMOS(false);
      }
      if (strcmp((char *)data, "relaisOutputSwitch_on") == 0)
      {
        relaisComparsionResult = true;
      }
      if (strcmp((char *)data, "relaisOutputSwitch_off") == 0)
      {
        relaisComparsionResult = false;
      }
      if (strcmp((char *)data, "wake_bms") == 0)
      {
        wakeupHandler(true);
        DEBUG_PRINTLN(F("<WEBS> wakeup manual from Web"));
        DEBUG_WEBLN(F("<WEBS> wakeup manual from Web"));
      }
      mqtttimer = (_settings.data.mqttRefresh * 1000) * (-1);
      webSocketAction = true;
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    wsClient = client;
    getJsonDevice();
    getJsonData();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    wsClient = nullptr;
    ws.cleanupClients();
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    wsClient = nullptr;
    ws.cleanupClients();
    break;
  }
}

bool wakeupHandler(bool wakeIt)
{
  if (wakeIt)
  {
    digitalWrite(WAKEUP_PIN, !digitalRead(WAKEUP_PIN));
    wakeuptimer = millis();
    DEBUG_PRINTLN(F("<SYS >Wakeup acivated"));
    DEBUG_WEBLN(F("<SYS > Wakeup acivated"));
  }
  if (millis() > (wakeuptimer + WAKEUP_DURATION) && wakeuptimer != 0)
  {
    digitalWrite(WAKEUP_PIN, !digitalRead(WAKEUP_PIN));
    wakeuptimer = 0;
    DEBUG_PRINTLN(F("<SYS >Wakeup deacivated"));
    DEBUG_WEBLN(F("<SYS > Wakeup deacivated"));
  }
  return true;
}

bool relaisHandler()
{
  if (_settings.data.relaisEnable && (millis() - relaistimer > RELAISINTERVAL))
  {
    relaistimer = millis();
    // read the value to compare to depending on the mode
    switch (_settings.data.relaisFunction)
    {
    case 0:
      // Mode 0 - Lowest Cell Voltage
      relaisCompareValueTmp = bms.get.minCellmV * 0.001;
      break;
    case 1:
      // Mode 1 - Highest Cell Voltage
      relaisCompareValueTmp = bms.get.maxCellmV * 0.001;
      break;
    case 2:
      // Mode 2 - Pack Voltage
      relaisCompareValueTmp = bms.get.packVoltage;
      break;
    case 3:
      // Mode 3 - Temperature
      relaisCompareValueTmp = bms.get.tempAverage;
      break;
    case 4:
      // Mode 4 - Manual per WEB or MQTT
      break;
    }

    if (!bms.get.connectionState)
      relaisCompareValueTmp = '\0';
    if (relaisCompareValueTmp == '\0' && _settings.data.relaisFunction != 4)
    {
      if (_settings.data.relaisFailsafe)
      {
        return false;
      }
      else
      {
        relaisComparsionResult = false;
        _settings.data.relaisInvert ? digitalWrite(RELAIS_PIN, !relaisComparsionResult) : digitalWrite(RELAIS_PIN, relaisComparsionResult);
      }
    }
    // now compare depending on the mode
    if (_settings.data.relaisFunction != 4)
    {
      // other modes
      switch (_settings.data.relaisComparsion)
      {
      case 0:
        // Higher or equal than
        // check if value is already true so we have to use hysteresis to switch off
        if (relaisComparsionResult)
        {
          relaisComparsionResult = relaisCompareValueTmp >= (_settings.data.relaisSetValue - _settings.data.relaisHysteresis) ? true : false;
        }
        else
        {
          // check if value is greater than
          relaisComparsionResult = relaisCompareValueTmp >= (_settings.data.relaisSetValue) ? true : false;
        }
        break;
      case 1:
        // Lower or equal than
        // check if value is already true so we have to use hysteresis to switch off
        if (relaisComparsionResult)
        {
          // use hystersis to switch off
          relaisComparsionResult = relaisCompareValueTmp <= (_settings.data.relaisSetValue + _settings.data.relaisHysteresis) ? true : false;
        }
        else
        {
          // check if value is greater than
          relaisComparsionResult = relaisCompareValueTmp <= (_settings.data.relaisSetValue) ? true : false;
        }
        break;
      }
    }
    else
    {
      // manual mode, currently no need to set anything, relaisComparsionResult is set by WEB or MQTT
      // i keep this just here for better reading of the code. The else {} statement can be removed later
    }

    _settings.data.relaisInvert ? digitalWrite(RELAIS_PIN, !relaisComparsionResult) : digitalWrite(RELAIS_PIN, relaisComparsionResult);

    return true;
  }
  return false;
}

bool resetCounter(bool count)
{

  if (count)
  {
    if (ESP.getResetInfoPtr()->reason == 6)
    {
      ESP.rtcUserMemoryRead(16, &bootcount, sizeof(bootcount));

      if (bootcount >= 10 && bootcount < 20)
      {
        // bootcount = 0;
        // ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
        _settings.reset();
        ESP.eraseConfig();
        ESP.reset();
      }
      else
      {
        bootcount++;
        ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
      }
    }
    else
    {
      bootcount = 0;
      ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
    }
  }
  else
  {
    bootcount = 0;
    ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
  }
  DEBUG_PRINT(F("Bootcount: "));
  DEBUG_PRINTLN(bootcount);
  DEBUG_PRINT(F("Reboot reason: "));
  DEBUG_PRINTLN(ESP.getResetInfoPtr()->reason);
  return true;
}

void setup()
{
  DEBUG_BEGIN(DEBUG_BAUD); // Debugging towards UART

  resetCounter(true);

  _settings.load();
  haAutoDiscTrigger = _settings.data.haDiscovery;
  pinMode(WAKEUP_PIN, OUTPUT);
  digitalWrite(WAKEUP_PIN, _settings.data.wakeupEnable);
  pinMode(RELAIS_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);
  WiFi.persistent(true); // fix wifi save bug
  WiFi.hostname(_settings.data.deviceName);
  deviceJson["Name"] = _settings.data.deviceName; // set the device name in json string

  sprintf(mqttClientId, "%s-%06X", _settings.data.deviceName, ESP.getChipId());

  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Device Name:\t"));
  DEBUG_PRINTLN(_settings.data.deviceName);
  DEBUG_PRINT(F("Mqtt Server:\t"));
  DEBUG_PRINTLN(_settings.data.mqttServer);
  DEBUG_PRINT(F("Mqtt Port:\t"));
  DEBUG_PRINTLN(_settings.data.mqttPort);
  DEBUG_PRINT(F("Mqtt User:\t"));
  DEBUG_PRINTLN(_settings.data.mqttUser);
  DEBUG_PRINT(F("Mqtt Passwort:\t"));
  DEBUG_PRINTLN(_settings.data.mqttPassword);
  DEBUG_PRINT(F("Mqtt Interval:\t"));
  DEBUG_PRINTLN(_settings.data.mqttRefresh);
  DEBUG_PRINT(F("Mqtt Topic:\t"));
  DEBUG_PRINTLN(_settings.data.mqttTopic);
  DEBUG_PRINT(F("wakeupEnable:\t"));
  DEBUG_PRINTLN(_settings.data.wakeupEnable);
  DEBUG_PRINT(F("relaisEnable:\t"));
  DEBUG_PRINTLN(_settings.data.relaisEnable);
  DEBUG_PRINT(F("relaisInvert:\t"));
  DEBUG_PRINTLN(_settings.data.relaisInvert);
  DEBUG_PRINT(F("relaisFunction:\t"));
  DEBUG_PRINTLN(_settings.data.relaisFunction);
  DEBUG_PRINT(F("relaisComparsion:\t"));
  DEBUG_PRINTLN(_settings.data.relaisComparsion);
  DEBUG_PRINT(F("relaisSetValue:\t"));
  DEBUG_PRINTLN(_settings.data.relaisSetValue, 3);
  DEBUG_PRINT(F("relaisHysteresis:\t"));
  DEBUG_PRINTLN(_settings.data.relaisHysteresis, 3);

  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", "BMS01", 32);
  AsyncWiFiManagerParameter custom_mqtt_triggerpath("mqtt_triggerpath", "MQTT Data Trigger Path", NULL, 80);
  AsyncWiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", "1883", 5);
  AsyncWiFiManagerParameter custom_mqtt_refresh("mqtt_refresh", "MQTT Send Interval", "300", 4);
  AsyncWiFiManagerParameter custom_device_name("device_name", "Device Name", "Daly2MQTT", 32);

  AsyncWiFiManager wm(&server, &dns);
  wm.setDebugOutput(false);       // disable wifimanager debug output
  wm.setMinimumSignalQuality(20); // filter weak wifi signals
  // wm.setConnectTimeout(15);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wm.setSaveConfigCallback(saveConfigCallback);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_triggerpath);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);

  bool apRunning = wm.autoConnect("Daly2MQTT-AP");

  // save settings if wifi setup is fire up
  if (shouldSaveConfig)
  {
    strncpy(_settings.data.mqttServer, custom_mqtt_server.getValue(), 40);
    strncpy(_settings.data.mqttUser, custom_mqtt_user.getValue(), 40);
    strncpy(_settings.data.mqttPassword, custom_mqtt_pass.getValue(), 40);
    _settings.data.mqttPort = atoi(custom_mqtt_port.getValue());
    strncpy(_settings.data.deviceName, custom_device_name.getValue(), 40);
    strncpy(_settings.data.mqttTopic, custom_mqtt_topic.getValue(), 40);
    _settings.data.mqttRefresh = atoi(custom_mqtt_refresh.getValue());
    strncpy(_settings.data.mqttTriggerPath, custom_mqtt_triggerpath.getValue(), 80);
    _settings.save();
    ESP.reset();
  }
  mqttclient.setServer(_settings.data.mqttServer, _settings.data.mqttPort);
  DEBUG_PRINTLN(F("<MQTT> MQTT Server config Loaded"));
  mqttclient.setCallback(mqttcallback);
  //  check is WiFi connected
  if (!apRunning)
  {
    DEBUG_PRINTLN(F("<SYS >Failed to connect to WiFi or hit timeout"));
  }
  else
  {
    // deviceJson["IP"] = WiFi.localIP(); // grab the device ip
    // bms.Init(); // init the bms driver
    // bms.callback(prozessData);

    // rebuild the py script and webserver to chunked response for faster react, example here
    // https://github.com/helderpe/espurna/blob/76ad9cde5a740822da9fe6e3f369629fa4b59ebc/code/espurna/web.ino
    // https://stackoverflow.com/questions/66717045/espasyncwebserver-chunked-response-inside-processor-function-esp32-esp8266
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_MAIN, htmlProcessor);
      request->send(response); });

    server.on("/livejson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      serializeJson(bmsJson, *response);
      request->send(response); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_REBOOT, htmlProcessor);
                request->send(response);
                restartNow = true;
                RestartTimer = millis(); });

    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_CONFIRM_RESET, htmlProcessor);
      request->send(response); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device is Erasing...");
      response->addHeader("Refresh", "15; url=/");
      response->addHeader("Connection", "close");
      request->send(response);
      delay(1000);
      _settings.reset();
      ESP.eraseConfig();
      ESP.reset(); });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS, htmlProcessor);
      request->send(response); });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS_EDIT, htmlProcessor);
      request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      strncpy(_settings.data.mqttServer, request->arg("post_mqttServer").c_str(), 40);
      _settings.data.mqttPort = request->arg("post_mqttPort").toInt();
      strncpy(_settings.data.mqttUser, request->arg("post_mqttUser").c_str(), 40);
      strncpy(_settings.data.mqttPassword, request->arg("post_mqttPassword").c_str(), 40);
      strncpy(_settings.data.mqttTopic, request->arg("post_mqttTopic").c_str(), 40);
      _settings.data.mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt(); // prevent lower numbers
      strncpy(_settings.data.mqttTriggerPath, request->arg("post_mqtttrigger").c_str(), 80);
      strncpy(_settings.data.deviceName, request->arg("post_deviceName").c_str(), 40);
      _settings.data.mqttJson = (request->arg("post_mqttjson") == "true") ? true : false;
      _settings.data.wakeupEnable = (request->arg("post_wakeupenable") == "true") ? true : false;
      _settings.data.relaisEnable = (request->arg("post_relaisenable") == "true") ? true : false;
      _settings.data.relaisInvert = (request->arg("post_relaisinvert") == "true") ? true : false;
      _settings.data.relaisFailsafe = (request->arg("post_relaisfailsafe") == "true") ? true : false;
      _settings.data.relaisFunction = request->arg("post_relaisfunction").toInt();
      _settings.data.relaisComparsion = request->arg("post_relaiscomparsion").toInt();
      _settings.data.relaisSetValue = request->arg("post_relaissetvalue").toFloat();
      _settings.data.relaisHysteresis = strtof(request->arg("post_relaishysteresis").c_str(), NULL);
      _settings.data.webUIdarkmode = (request->arg("post_webuicolormode") == "true") ? true : false;
      strncpy(_settings.data.httpUser, request->arg("post_httpUser").c_str(), 40);
      strncpy(_settings.data.httpPass, request->arg("post_httpPass").c_str(), 40);
      _settings.data.haDiscovery = (request->arg("post_hadiscovery") == "true") ? true : false;

      _settings.save();
      request->redirect("/reboot"); });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
      AsyncWebParameter *p = request->getParam(0);
      if (p->name() == "chargefet")
      {
        DEBUG_PRINTLN(F("<WEBS> Webcall: charge fet to: ")+(String)p->value());
        // DEBUG_WEBLN(F("<WEBS> Webcall: charge fet to: ")+(String)p->value());
        if(p->value().toInt() == 1){
          bms.setChargeMOS(true);
          bms.get.chargeFetState = true;
        }
        if(p->value().toInt() == 0){
          bms.setChargeMOS(false);
          bms.get.chargeFetState = false;
        }
      }
      if (p->name() == "dischargefet")
      {
        DEBUG_PRINTLN(F("<WEBS> Webcall: discharge fet to: ")+(String)p->value());
        // DEBUG_WEBLN(F("<WEBS> Webcall: discharge fet to: ")+(String)p->value());
        if(p->value().toInt() == 1){
          bms.setDischargeMOS(true);
          bms.get.disChargeFetState = true;
        }
        if(p->value().toInt() == 0){
          bms.setDischargeMOS(false);
          bms.get.disChargeFetState = false;
        }
      }
      if (p->name() == "soc")
      {
        DEBUG_PRINTLN(F("<WEBS> Webcall: setsoc SOC set to: ")+(String)p->value());
        // DEBUG_WEBLN(F("<WEBS> Webcall: setsoc SOC set to: ")+(String)p->value());
        if(p->value().toInt() >= 0 && p->value().toInt() <= 100 ){
          bms.setSOC(p->value().toInt());
        }
      }
      if (p->name() == "relais")
      {
        DEBUG_PRINTLN(F("<WEBS> Webcall: set relais to: ")+(String)p->value());
        // DEBUG_WEBLN(F("<WEBS> Webcall: set relais to: ")+(String)p->value());
        if(p->value() == "true"){
          relaisComparsionResult = true;
        }
        if(p->value().toInt() == 0){
          relaisComparsionResult = false;
        }
      }
        if (p->name() == "bmsreset")
        {
          DEBUG_PRINTLN(F("<WEBS> Webcall: reset BMS"));
          // DEBUG_WEBLN(F("<WEBS> Webcall: reset BMS"));
          if(p->value().toInt() == 1){
            bms.setBmsReset();
          }
        }
        if (p->name() == "bmswake")
        {
          if(p->value().toInt() == 1){
            wakeupHandler(true);
            DEBUG_PRINTLN(F("<WEBS> wakeup manual from Web"));
            // DEBUG_WEBLN(F("<WEBS> wakeup manual from Web"));
          }
        }
        if (p->name() == "ha")
        {
          haDiscTrigger = true;
        }
        request->send(200, "text/plain", "message received"); });

    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
          if(strlen(_settings.data.httpUser) > 0 && !request->authenticate(_settings.data.httpUser, _settings.data.httpPass)) return request->requestAuthentication();
    //https://gist.github.com/JMishou/60cb762047b735685e8a09cd2eb42a60
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
          // Upload handler chunks in data

          if (!index)
          { // if index == 0 then this is the first frame of data
            Serial.printf("UploadStart: %s\n", filename.c_str());
            Serial.setDebugOutput(true);

            // calculate sketch space required for the update
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace))
            { // start with max available size
              Update.printError(Serial);
            }
            Update.runAsync(true); // tell the updaterClass to run in async mode
          }

          // Write chunked data to the free sketch space
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }

          if (final)
          { // if the final flag is set then this is the last frame of data
            if (Update.end(true))
            { // true to set the size to the current progress
              Serial.printf("Update Success: %u B\nRebooting...\n", index + len);
            }
            else
            {
              Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
          }
        });

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(418, "text/plain", "418 I'm a teapot"); });

    // set the device name
    MDNS.addService("http", "tcp", 80);
    if (MDNS.begin(_settings.data.deviceName))
    {
      DEBUG_PRINTLN(F("<SYS > mDNS running..."));
      MDNS.update();
    }
    ws.onEvent(onEvent);
    server.addHandler(&ws);
#ifdef isDEBUG
    // WebSerial is accessible at "<IP Address>/webserial" in browser
    WebSerial.begin(&server);
    /* Attach Message Callback */
    // WebSerial.onMessage(webSrecvMsg);
#endif

    server.begin();
    DEBUG_PRINTLN(F("<SYS > Webserver Running..."));

    mqtttimer = (_settings.data.mqttRefresh * 1000) * (-1);

    deviceJson["IP"] = WiFi.localIP(); // grab the device ip
    bms.Init();                        // init the bms driver
    bms.callback(prozessData);

    tempSens.begin();
    numOfTempSens = tempSens.getDeviceCount();
  }
  analogWrite(LED_PIN, 255);
  resetCounter(false);
}
// end void setup
void loop()
{
  if (Update.isRunning())
  {
    workerCanRun = false; // lockout, atfer true need reboot
  }
  if (workerCanRun)
  {
    // Make sure wifi is in the right mode
    if (WiFi.status() == WL_CONNECTED)
    {
      ws.cleanupClients(); // clean unused client connections
      MDNS.update();
      mqttclient.loop(); // Check if we have something to read from MQTT
      if (haDiscTrigger || haAutoDiscTrigger)
      {
        if (sendHaDiscovery())
        {
          haDiscTrigger = false;
          haAutoDiscTrigger = false;
        }
      }
    }
    bms.loop();
    wakeupHandler(false);
    relaisHandler();
    notificationLED();
  }
  if (restartNow && millis() >= (RestartTimer + 500))
  {
    DEBUG_PRINTLN("<SYS > Restart");
    ESP.reset();
  }
}
// End void loop
void prozessData()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    tempSens.requestTemperatures();
    getJsonDevice();
    getJsonData();
    if (wsClient != nullptr && wsClient->canSend())
    {
      notifyClients();
    }
    if (millis() - mqtttimer > (_settings.data.mqttRefresh * 1000)  || mqtttimer == 0)
    {
      sendtoMQTT();
      mqtttimer = millis();
    }
  }
}

void getJsonDevice()
{
  deviceJson[F("ESP_VCC")] = (ESP.getVcc() / 1000.0) + 0.3;
  deviceJson[F("Wifi_RSSI")] = WiFi.RSSI();
  deviceJson[F("Relais_Active")] = relaisComparsionResult ? true : false;
  deviceJson[F("Relais_Manual")] = _settings.data.relaisEnable && _settings.data.relaisFunction == 4 ? true : false;
  deviceJson[F("sw_version")] = SOFTWARE_VERSION;
  deviceJson[F("Flash_Size")] = ESP.getFlashChipSize();
  deviceJson[F("Sketch_Size")] = ESP.getSketchSize();
  deviceJson[F("Free_Sketch_Space")] = ESP.getFreeSketchSpace();
#ifdef DALY_BMS_DEBUG
  deviceJson[F("CPU_Frequency")] = ESP.getCpuFreqMHz();
  deviceJson[F("Real_Flash_Size")] = ESP.getFlashChipRealSize();
  deviceJson[F("Free_Heap")] = ESP.getFreeHeap();
  deviceJson[F("HEAP_Fragmentation")] = ESP.getHeapFragmentation();
  deviceJson[F("Free_BlockSize")] = ESP.getMaxFreeBlockSize();
  deviceJson[F("json_memory_usage")] = bmsJson.memoryUsage();
  deviceJson[F("json_capacity")] = bmsJson.capacity();
  deviceJson[F("runtime")] = millis() / 1000;
  deviceJson[F("ws_clients")] = ws.count();
  deviceJson[F("MQTT_Json")] = _settings.data.mqttJson;
#endif
}

void getJsonData()
{
  packJson[F("Voltage")] = bms.get.packVoltage;
  packJson[F("Current")] = bms.get.packCurrent;
  packJson[F("Power")] = (bms.get.packCurrent * bms.get.packVoltage);
  packJson[F("SOC")] = bms.get.packSOC;
  packJson[F("Remaining_Ah")] = bms.get.resCapacityAh;
  packJson[F("Cycles")] = bms.get.bmsCycles;
  packJson[F("BMS_Temp")] = bms.get.tempAverage;
  packJson[F("Cell_Temp")] = bms.get.cellTemperature[0];
  packJson[F("cell_hVt")] = bms.get.maxCellThreshold1 / 1000;
  packJson[F("cell_lVt")] = bms.get.minCellThreshold1 / 1000;
  packJson[F("cell_hVt2")] = bms.get.maxCellThreshold2 / 1000;
  packJson[F("cell_lVt2")] = bms.get.minCellThreshold2 / 1000;
  packJson[F("pack_hVt")] = bms.get.maxPackThreshold1 / 10;
  packJson[F("pack_lVt")] = bms.get.minPackThreshold1 / 10;
  packJson[F("pack_hVt2")] = bms.get.maxPackThreshold2 / 10;
  packJson[F("pack_lVt2")] = bms.get.minPackThreshold2 / 10;
  packJson[F("High_CellNr")] = bms.get.maxCellVNum;
  packJson[F("High_CellV")] = bms.get.maxCellmV / 1000;
  packJson[F("Low_CellNr")] = bms.get.minCellVNum;
  packJson[F("Low_CellV")] = bms.get.minCellmV / 1000;
  packJson[F("Cell_Diff")] = bms.get.cellDiff;
  packJson[F("DischargeFET")] = bms.get.disChargeFetState ? true : false;
  packJson[F("ChargeFET")] = bms.get.chargeFetState ? true : false;
  packJson[F("Status")] = bms.get.chargeDischargeStatus;
  packJson[F("Cells")] = bms.get.numberOfCells;
  packJson[F("Heartbeat")] = bms.get.bmsHeartBeat;
  packJson[F("Balance_Active")] = bms.get.cellBalanceActive ? true : false;
  packJson[F("Fail_Codes")] = bms.failCodeArr;

  for (int i = 0; i < numOfTempSens; i++)
  {
    if (tempSens.getAddress(tempDeviceAddress, i))
    {
      packJson["DS18B20_" + String(i + 1)] = tempSens.getTempC(tempDeviceAddress);
    }
  }

  for (size_t i = 0; i < size_t(bms.get.numberOfCells); i++)
  {
    cellVJson[F("CellV_") + String(i + 1)] = bms.get.cellVmV[i] / 1000;
    cellVJson[F("Balance_") + String(i + 1)] = bms.get.cellBalanceState[i];
  }

  for (size_t i = 0; i < size_t(bms.get.numOfTempSensors); i++)
  {
    cellTempJson[F("Cell_Temp_") + String(i + 1)] = bms.get.cellTemperature[i];
  }
}

char *topicBuilder(char *buffer, char const *path, char const *numering = "")
{                                                   // buffer, topic
  const char *mainTopic = _settings.data.mqttTopic; // get the main topic path

  strcpy(buffer, mainTopic);
  strcat(buffer, "/");
  strcat(buffer, path);
  strcat(buffer, numering);
  return buffer;
}

bool sendtoMQTT()
{
  char msgBuffer[32];
  char buff[256]; // temp buffer for the topic string
  if (!connectMQTT())
  {
    DEBUG_PRINTLN(F("<MQTT> Error: No connection to MQTT Server, cant send Data!"));
    DEBUG_WEBLN(F("<MQTT> Error: No connection to MQTT Server, cant send Data!"));
    firstPublish = false;
    return false;
  }
  DEBUG_PRINT(F("<MQTT> Data sent to MQTT Server... "));
  DEBUG_WEB(F("<MQTT> Data sent to MQTT Server... "));
  mqttclient.publish(topicBuilder(buff, "Alive"), "true", true); // LWT online message must be retained!
  mqttclient.publish(topicBuilder(buff, "Wifi_RSSI"), String(WiFi.RSSI()).c_str());
  mqttclient.publish(topicBuilder(buff, "sw_version"), SOFTWARE_VERSION);
  if (!_settings.data.mqttJson)
  {
    mqttclient.publish(topicBuilder(buff, "Pack_Voltage"), dtostrf(bms.get.packVoltage, 4, 1, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_Current"), dtostrf(bms.get.packCurrent, 4, 1, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_Power"), dtostrf((bms.get.packVoltage * bms.get.packCurrent), 4, 1, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_SOC"), dtostrf(bms.get.packSOC, 4, 1, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_Remaining_mAh"), dtostrf(bms.get.resCapacityAh, 3, 1, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_Cycles"), itoa(bms.get.bmsCycles, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_BMS_Temperature"), itoa(bms.get.tempAverage, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_Cell_High"), itoa(bms.get.maxCellVNum, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_Cell_Low"), itoa(bms.get.minCellVNum, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_Cell_High_Voltage"), dtostrf(bms.get.maxCellmV / 1000, 5, 3, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_Cell_Low_Voltage"), dtostrf(bms.get.minCellmV / 1000, 5, 3, msgBuffer));
    mqttclient.publish(topicBuilder(buff, "Pack_Cell_Difference"), itoa(bms.get.cellDiff, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_ChargeFET"), bms.get.chargeFetState ? "true" : "false");
    mqttclient.publish(topicBuilder(buff, "Pack_DischargeFET"), bms.get.disChargeFetState ? "true" : "false");
    mqttclient.publish(topicBuilder(buff, "Pack_Status"), bms.get.chargeDischargeStatus);
    mqttclient.publish(topicBuilder(buff, "Pack_Cells"), itoa(bms.get.numberOfCells, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_Heartbeat"), itoa(bms.get.bmsHeartBeat, msgBuffer, 10));
    mqttclient.publish(topicBuilder(buff, "Pack_Balance_Active"), bms.get.cellBalanceActive ? "true" : "false");
    mqttclient.publish(topicBuilder(buff, "Pack_Failure"), bms.failCodeArr.c_str());

    for (size_t i = 0; i < bms.get.numberOfCells; i++)
    {
      mqttclient.publish(topicBuilder(buff, "Pack_Cells_Voltage/Cell_", itoa((i + 1), msgBuffer, 10)), dtostrf(bms.get.cellVmV[i] / 1000, 5, 3, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Pack_Cells_Balance/Cell_", itoa((i + 1), msgBuffer, 10)), bms.get.cellBalanceState[i] ? "true" : "false");
    }
    for (size_t i = 0; i < bms.get.numOfTempSensors; i++)
    {
      mqttclient.publish(topicBuilder(buff, "Pack_Cell_Temperature_", itoa((i + 1), msgBuffer, 10)), itoa(bms.get.cellTemperature[i], msgBuffer, 10));
    }
    mqttclient.publish(topicBuilder(buff, "Pack_Relais"), relaisComparsionResult ? "true" : "false");
    mqttclient.publish(topicBuilder(buff, "Pack_Relais_Manual"), (_settings.data.relaisFunction == 4) ? "true" : "false"); // should we keep this? you can check with iobroker etc. if you can even switch the relais using mqtt
    for (int i = 0; i < numOfTempSens; i++)
    {
      if (tempSens.getAddress(tempDeviceAddress, i))
      {
        mqttclient.publish(topicBuilder(buff, "DS18B20_", itoa((i + 1), msgBuffer, 10)), dtostrf(tempSens.getTempC(tempDeviceAddress), 4, 2, msgBuffer));
      }
    }
  }
  else
  {
    mqttclient.beginPublish(topicBuilder(buff, "Pack_Data"), measureJson(bmsJson), false);
    serializeJson(bmsJson, mqttclient);
    mqttclient.endPublish();
  }
  DEBUG_PRINTLN(F("Done"));
  DEBUG_WEBLN(F("Done"));
  firstPublish = true;

  return true;
}

void mqttcallback(char *topic, unsigned char *payload, unsigned int length)
{
  char buff[256];
  if (firstPublish == false)
    return;

  // updateProgress = true;

  String messageTemp;
  for (unsigned int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }

  // check if the message not empty
  if (messageTemp.length() <= 0)
  {
    DEBUG_PRINTLN(F("<MQTT> MQTT Callback: message empty, break!"));
    DEBUG_WEBLN(F("<MQTT> MQTT Callback: message empty, break!"));
    // updateProgress = false;
    return;
  }
  DEBUG_PRINTLN(F("<MQTT> MQTT Callback: message recived: ") + messageTemp);
  DEBUG_WEBLN(F("<MQTT> MQTT Callback: message recived: ") + messageTemp);
  // set Relais
  if (strcmp(topic, topicBuilder(buff, "Device_Control/Pack_Relais")) == 0)
  {
    if (_settings.data.relaisFunction == 4 && messageTemp == "true")
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: switching Relais on"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: switching Relais on"));
      relaisComparsionResult = true;
      mqttclient.publish(topicBuilder(buff, "Pack_Relais"), "true", false);
      mqtttimer = 0;
      relaisHandler();
    }
    if (_settings.data.relaisFunction == 4 && messageTemp == "false")
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: switching Relais off"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: switching Relais off"));
      relaisComparsionResult = false;
      mqttclient.publish(topicBuilder(buff, "Pack_Relais"), "false", false);
      mqtttimer = 0;
      relaisHandler();
    }
  }
  // Wake BMS
  if (strcmp(topic, topicBuilder(buff, "Device_Control/Wake_BMS")) == 0)
  {
    if (messageTemp == "true")
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: wakeup manual from Web"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: wakeup manual from Web"));
      mqttclient.publish(topicBuilder(buff, "Device_Control/Wake_BMS"), "false", false);
      mqtttimer = 0;
      wakeupHandler(true);
    }
  }
  // set SOC
  if (strcmp(topic, topicBuilder(buff, "Device_Control/Pack_SOC")) == 0)
  {
    if (bms.get.packSOC != atof(messageTemp.c_str()) && atof(messageTemp.c_str()) >= 0 && atof(messageTemp.c_str()) <= 100)
    {
      if (bms.setSOC(atof(messageTemp.c_str())))
      {
        DEBUG_PRINTLN(F("<MQTT> MQTT Callback: SOC message OK, Write: ") + messageTemp);
        DEBUG_WEBLN(F("<MQTT> MQTT Callback: SOC message OK, Write: ") + messageTemp);
        mqttclient.publish(topicBuilder(buff, "Pack_SOC"), String(atof(messageTemp.c_str())).c_str(), false);
        mqtttimer = 0;
      }
    }
  }

  // Switch the Discharging port
  if (strcmp(topic, topicBuilder(buff, "Device_Control/Pack_DischargeFET")) == 0)
  {
    DEBUG_PRINTLN(F("<MQTT> message recived: ") + messageTemp);
    DEBUG_WEBLN(F("<MQTT> message recived: ") + messageTemp);
    if (messageTemp == "true" && !bms.get.disChargeFetState)
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: switching Discharging mos on"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: switching Discharging mos on"));
      if (bms.setDischargeMOS(true))
      {
        mqttclient.publish(topicBuilder(buff, "Pack_DischargeFET"), "true", false);
        mqtttimer = 0;
      }
    }
    if (messageTemp == "false" && bms.get.disChargeFetState)
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: switching Discharging mos off"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: switching Discharging mos off"));
      if (bms.setDischargeMOS(false))
      {
        mqttclient.publish(topicBuilder(buff, "Pack_DischargeFET"), "false", false);
        mqtttimer = 0;
      }
    }
  }

  // Switch the Charging Port
  if (strcmp(topic, topicBuilder(buff, "Device_Control/Pack_ChargeFET")) == 0)
  {
    DEBUG_PRINTLN(F("<MQTT> message recived: ") + messageTemp);
    DEBUG_WEBLN(F("<MQTT> message recived: ") + messageTemp);
    if (messageTemp == "true" && !bms.get.chargeFetState)
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: switching Charging mos on"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: switching Charging mos on"));
      if (bms.setChargeMOS(true))
      {
        mqttclient.publish(topicBuilder(buff, "Pack_ChargeFET"), "true", false);
        mqtttimer = 0;
      }
    }
    if (messageTemp == "false" && bms.get.chargeFetState)
    {
      DEBUG_PRINTLN(F("<MQTT> MQTT Callback: switching Charging mos off"));
      DEBUG_WEBLN(F("<MQTT> MQTT Callback: switching Charging mos off"));
      if (bms.setChargeMOS(false))
      {
        mqttclient.publish(topicBuilder(buff, "Pack_ChargeFET"), "false", false);
        mqtttimer = 0;
      }
    }
  }

  if (strlen(_settings.data.mqttTriggerPath) > 0 && strcmp(topic, _settings.data.mqttTriggerPath) == 0)
  {
    DEBUG_PRINTLN(F("<MQTT> MQTT Data Trigger Firered Up"));
    DEBUG_WEBLN(F("<MQTT> MQTT Data Trigger Firered Up"));
    mqtttimer = 0;
  }
}

bool connectMQTT()
{
  char buff[256];
  if (!mqttclient.connected() && strlen(_settings.data.mqttServer) > 0)
  {
    firstPublish = false;
    DEBUG_PRINT(F("<MQTT> MQTT Client State is: "));
    DEBUG_WEB(F("<MQTT> MQTT Client State is: "));
    DEBUG_PRINTLN(mqttclient.state());
    DEBUG_WEBLN(mqttclient.state());
    DEBUG_PRINT(F("<MQTT> establish MQTT Connection... "));
    DEBUG_WEB(F("<MQTT> establish MQTT Connection... "));

    if (mqttclient.connect(mqttClientId, _settings.data.mqttUser, _settings.data.mqttPassword, (topicBuilder(buff, "Alive")), 0, true, "false", true))
    {
      if (mqttclient.connected())
      {
        DEBUG_PRINTLN(F("Done"));
        DEBUG_WEBLN(F("Done"));
        mqttclient.publish(topicBuilder(buff, "Alive"), "true", true); // LWT online message must be retained!
        mqttclient.publish(topicBuilder(buff, "Device_IP"), (const char *)(WiFi.localIP().toString()).c_str(), true);
        mqttclient.subscribe(topicBuilder(buff, "Device_Control/Pack_DischargeFET"));
        mqttclient.subscribe(topicBuilder(buff, "Device_Control/Pack_ChargeFET"));
        mqttclient.subscribe(topicBuilder(buff, "Device_Control/Pack_SOC"));
        mqttclient.subscribe(topicBuilder(buff, "Device_Control/Wake_BMS"));

        if (strlen(_settings.data.mqttTriggerPath) > 0)
        {
          mqttclient.subscribe(_settings.data.mqttTriggerPath);
        }

        if (_settings.data.relaisFunction == 4)
          mqttclient.subscribe(topicBuilder(buff, "Device_Control/Pack_Relais"));
      }
      else
      {
        DEBUG_PRINTLN(F("Fail\n"));
        DEBUG_WEBLN(F("Fail\n"));
      }
    }
    else
    {
      DEBUG_PRINTLN(F("Fail\n"));
      DEBUG_WEBLN(F("Fail\n"));
      return false; // Exit if we couldnt connect to MQTT brooker
    }
    firstPublish = true;
  }
  return true;
}

bool sendHaDiscovery()
{
  if (!bms.getState() || bms.get.numberOfCells == 0)
  {
    return false;
  }
  if (!connectMQTT())
  {
    return false;
  }
  String haDeviceDescription = String("\"dev\":") +
                               "{\"ids\":[\"" + mqttClientId + "\"]," +
                               "\"name\":\"" + _settings.data.deviceName + "\"," +
                               "\"cu\":\"http://" + WiFi.localIP().toString() + "\"," +
                               "\"mdl\":\"Daly2MQTT\"," +
                               "\"mf\":\"SoftWareCrash\"," +
                               "\"sw\":\"" + SOFTWARE_VERSION + "\"" +
                               "}";

  char topBuff[128];
  // main pack data
  for (size_t i = 0; i < sizeof haPackDescriptor / sizeof haPackDescriptor[0]; i++)
  {
    String haPayLoad = String("{") +
                       "\"name\":\"" + haPackDescriptor[i][0] + "\"," +
                       "\"stat_t\":\"" + _settings.data.mqttTopic + "/" + haPackDescriptor[i][0] + "\"," +
                       "\"uniq_id\":\"" + mqttClientId + "." + haPackDescriptor[i][0] + "\"," +
                       "\"ic\":\"mdi:" + haPackDescriptor[i][1] + "\",";
    if (strlen(haPackDescriptor[i][2]) != 0)
      haPayLoad += (String) "\"unit_of_meas\":\"" + haPackDescriptor[i][2] + "\",";
    if (strlen(haPackDescriptor[i][3]) != 0)
      haPayLoad += (String) "\"dev_cla\":\"" + haPackDescriptor[i][3] + "\",";
    haPayLoad += haDeviceDescription;
    haPayLoad += "}";
    sprintf(topBuff, "homeassistant/sensor/%s/%s/config", _settings.data.mqttTopic, haPackDescriptor[i][0]); // build the topic
    mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
    for (size_t i = 0; i < haPayLoad.length(); i++)
    {
      mqttclient.write(haPayLoad[i]);
    }
    mqttclient.endPublish();
  }
  // Cell data and balances
  for (size_t i = 0; i < bms.get.numberOfCells; i++)
  {

    String haPayLoad = String("{") +
                       "\"name\":\"Cell_Voltage_" + (i + 1) + "\"," +
                       "\"stat_t\":\"" + _settings.data.mqttTopic + "/Pack_Cells_Voltage/Cell_" + (i + 1) + "\"," +
                       "\"uniq_id\":\"" + mqttClientId + ".CellV_" + (i + 1) + "\"," +
                       "\"ic\":\"mdi:flash-triangle-outline\"," +
                       "\"unit_of_meas\":\"V\"," +
                       "\"dev_cla\":\"voltage\",";
    haPayLoad += haDeviceDescription;
    haPayLoad += "}";
    sprintf(topBuff, "homeassistant/sensor/%s/Cell_%d_Voltage/config", _settings.data.mqttTopic, (i + 1)); // build the topic

    mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
    for (size_t i = 0; i < haPayLoad.length(); i++)
    {
      mqttclient.write(haPayLoad[i]);
    }
    mqttclient.endPublish();

    haPayLoad = String("{") +
                "\"name\":\"Cell_balance_" + (i + 1) + "\"," +
                "\"stat_t\":\"" + _settings.data.mqttTopic + "/Pack_Cells_Balance/Cell_" + (i + 1) + "\"," +
                "\"uniq_id\":\"" + mqttClientId + ".CellB_" + (i + 1) + "\"," +
                "\"ic\":\"mdi:scale-balance\",";
    haPayLoad += haDeviceDescription;
    haPayLoad += "}";
    sprintf(topBuff, "homeassistant/sensor/%s/Cell_%d_Balance/config", _settings.data.mqttTopic, (i + 1)); // build the topic

    mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
    for (size_t i = 0; i < haPayLoad.length(); i++)
    {
      mqttclient.write(haPayLoad[i]);
    }
    mqttclient.endPublish();
  }
  // Ext Temp sensors
  for (int i = 0; i < numOfTempSens; i++)
  {
    if (tempSens.getAddress(tempDeviceAddress, i))
    {
      String haPayLoad = String("{") +
                         "\"name\":\"DS18B20_" + (i + 1) + "\"," +
                         "\"stat_t\":\"" + _settings.data.mqttTopic + "/DS18B20_" + (i + 1) + "\"," +
                         "\"uniq_id\":\"" + mqttClientId + ".DS18B20_" + (i + 1) + "\"," +
                         "\"ic\":\"mdi:thermometer-lines\"," +
                         "\"unit_of_meas\":\"°C\"," +
                         "\"dev_cla\":\"temperature\",";
      haPayLoad += haDeviceDescription;
      haPayLoad += "}";
      sprintf(topBuff, "homeassistant/sensor/%s/DS18B20_%d/config", _settings.data.mqttTopic, (i + 1)); // build the topic

      mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
      for (size_t i = 0; i < haPayLoad.length(); i++)
      {
        mqttclient.write(haPayLoad[i]);
      }
      mqttclient.endPublish();
    }
  }
  // temp sensors
  for (size_t i = 0; i < bms.get.numOfTempSensors; i++)
  {

    String haPayLoad = String("{") +
                       "\"name\":\"Pack_Cell_Temperature_" + (i + 1) + "\"," +
                       "\"stat_t\":\"" + _settings.data.mqttTopic + "/Pack_Cell_Temperature_" + (i + 1) + "\"," +
                       "\"uniq_id\":\"" + mqttClientId + ".Pack_Cell_Temperature_" + (i + 1) + "\"," +
                       "\"ic\":\"mdi:thermometer-lines\"," +
                       "\"unit_of_meas\":\"°C\"," +
                       "\"dev_cla\":\"temperature\",";
    haPayLoad += haDeviceDescription;
    haPayLoad += "}";
    sprintf(topBuff, "homeassistant/sensor/%s/Pack_Cell_Temperature_%d/config", _settings.data.mqttTopic, (i + 1)); // build the topic

    mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
    for (size_t i = 0; i < haPayLoad.length(); i++)
    {
      mqttclient.write(haPayLoad[i]);
    }
    mqttclient.endPublish();
  }

  // switches
  for (size_t i = 0; i < sizeof haControlDescriptor / sizeof haControlDescriptor[0]; i++)
  {
    String haPayLoad = String("{") +
                       "\"name\":\"" + haControlDescriptor[i][0] + "\"," +
                       "\"command_topic\":\"" + _settings.data.mqttTopic + "/Device_Control/" + haControlDescriptor[i][0] + "\"," +
                       "\"stat_t\":\"" + _settings.data.mqttTopic + "/" + haControlDescriptor[i][0] + "\"," +
                       "\"uniq_id\":\"" + mqttClientId + "." + haControlDescriptor[i][0] + "\"," +
                       "\"ic\":\"mdi:" + haControlDescriptor[i][1] + "\"," +
                       "\"pl_on\":\"true\"," +
                       "\"pl_off\":\"false\"," +
                       "\"stat_on\":\"true\"," +
                       "\"stat_off\":\"false\",";

    haPayLoad += haDeviceDescription;
    haPayLoad += "}";
    sprintf(topBuff, "homeassistant/switch/%s/%s/config", _settings.data.mqttTopic, haControlDescriptor[i][0]); // build the topic

    mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
    for (size_t i = 0; i < haPayLoad.length(); i++)
    {
      mqttclient.write(haPayLoad[i]);
    }
    mqttclient.endPublish();
  }
  return true;
};