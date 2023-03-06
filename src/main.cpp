// #include <Arduino.h>
/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

#include "main.h"
#include <daly-bms-uart.h> // This is where the library gets pulled in

#include "display.h"

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Settings.h"

#include "webpages/htmlCase.h"      // The HTML Konstructor
#include "webpages/main.h"          // landing page with menu
#include "webpages/settings.h"      // settings page
#include "webpages/settingsedit.h"  // mqtt settings page
#include "webpages/htmlProzessor.h" // The html Prozessor

WiFiClient client;
Settings _settings;
PubSubClient mqttclient(client);

StaticJsonDocument<JSON_BUFFER> bmsJson; // main Json
// DynamicJsonDocument bmsJson(JSON_BUFFER);                         // main Json
JsonObject deviceJson = bmsJson.createNestedObject("Device");     // basic device data
JsonObject packJson = bmsJson.createNestedObject("Pack");         // battery package data
JsonObject cellVJson = bmsJson.createNestedObject("CellV");       // nested data for cell voltages
JsonObject cellTempJson = bmsJson.createNestedObject("CellTemp"); // nested data for cell temp

String topicStrg;
int mqttdebug;

unsigned long mqtttimer = 0;
unsigned long bmstimer = 0;
unsigned long RestartTimer = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;
Daly_BMS_UART bms(MYPORT_RX, MYPORT_TX);

#include "status-LED.h"

// flag for saving data and other things
bool shouldSaveConfig = false;
bool restartNow = false;
bool updateProgress = false;
bool dataCollect = false;
bool firstPublish = false;
unsigned long wakeuptimer = WAKEUP_INTERVAL; // dont run immediately after boot, wait for first intervall
bool wakeupPinActive = false;

unsigned long relaistimer = RELAISINTERVAL; // dont run immediately after boot, wait for first intervall
float relaisCompareValueTmp = 0;
bool relaisComparsionResult = false;

char mqttClientId[80];

ADC_MODE(ADC_VCC);

//----------------------------------------------------------------------
void saveConfigCallback()
{

  DEBUG_PRINTLN(F("Should save config"));
  shouldSaveConfig = true;
}

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{

  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!index)
  {
    DEBUG_PRINTLN(F("Starting Firmware Update"));
    Update.runAsync(true);
    if (!Update.begin(free_space, U_FLASH))
    {
#ifdef isDEBUG
      Update.printError(DALY_BMS_DEBUG);
#endif
      ESP.restart();
    }
  }

  if (Update.write(data, len) != len)
  {
#ifdef isDEBUG
    Update.printError(DALY_BMS_DEBUG);
#endif
    ESP.restart();
  }

  if (final)
  {
    if (!Update.end(true))
    {
#ifdef isDEBUG
      Update.printError(DALY_BMS_DEBUG);
#endif
      ESP.restart();
    }
    else
    {
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device is booting new Firmware");
      response->addHeader("Refresh", "10; url=/");
      response->addHeader("Connection", "close");
      request->send(response);
      DEBUG_PRINTLN(F("Update complete"));
      RestartTimer = millis();
      restartNow = true; // Set flag so main loop can issue restart call
    }
  }
}

void notifyClients()
{
  if (wsClient != nullptr && wsClient->canSend())
  {
    DEBUG_PRINT(F("Info: Data sent to WebSocket... "));
    char data[JSON_BUFFER];
    size_t len = serializeJson(bmsJson, data);
    wsClient->text(data, len);
    DEBUG_PRINT(F("Done\n"));
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    updateProgress = true;
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
    updateProgress = false;
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
    break;
  case WS_EVT_DATA:
    bmstimer = millis();
    mqtttimer = millis();
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

bool wakeupHandler()
{
  if (_settings.data.wakeupEnable && (millis() > wakeuptimer))
  {
    DEBUG_PRINTLN();
    DEBUG_PRINTLN(F("wakeupHandler()"));
    DEBUG_PRINT(F("this run:\t"));
    DEBUG_PRINTLN(millis());
    DEBUG_PRINT(F("next run:\t"));
    DEBUG_PRINTLN(wakeuptimer);
    if (wakeupPinActive)
    {
      wakeupPinActive = false;
      wakeuptimer = millis() + WAKEUP_INTERVAL;
      digitalWrite(WAKEUP_PIN, LOW);
    }
    else
    {
      wakeupPinActive = true;
      wakeuptimer = millis() + WAKEUP_DURATION;
      digitalWrite(WAKEUP_PIN, HIGH);
    }
    DEBUG_PRINT(F("PIN IS NOW:\t"));
    DEBUG_PRINTLN(digitalRead(WAKEUP_PIN));
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
      relaisCompareValueTmp = bms.get.minCellmV / 1000;
      break;
    case 1:
      // Mode 1 - Highest Cell Voltage
      relaisCompareValueTmp = bms.get.maxCellmV / 1000;
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
    // if(relaisCompareValueTmp == NULL){
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

void setup()
{
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  DEBUG_BEGIN(9600); // Debugging towards UART1
  _settings.load();

  pinMode(WAKEUP_PIN, OUTPUT);
  pinMode(RELAIS_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 255);
  WiFi.persistent(true);                          // fix wifi save bug
  deviceJson["Name"] = _settings.data.deviceName; // set the device name in json string

  topicStrg = _settings.data.mqttTopic;

  sprintf(mqttClientId, "%s-%06X", _settings.data.deviceName, ESP.getChipId());

  AsyncWiFiManager wm(&server, &dns);
  wm.setDebugOutput(false);       // disable wifimanager debug output
  wm.setMinimumSignalQuality(10); // filter weak wifi signals
  wm.setSaveConfigCallback(saveConfigCallback);
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
  DEBUG_PRINTLN(_settings.data.relaisSetValue);
  DEBUG_PRINT(F("relaisHysteresis:\t"));
  DEBUG_PRINTLN(_settings.data.relaisHysteresis);
  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 32);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", "BMS01", 32);
  AsyncWiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", "1883", 5);
  AsyncWiFiManagerParameter custom_mqtt_refresh("mqtt_refresh", "MQTT Send Interval", "300", 4);
  AsyncWiFiManagerParameter custom_device_name("device_name", "Device Name", "DALY-BMS-to-MQTT", 32);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);

  bool res = wm.autoConnect("DALY-BMS-AP");

  wm.setConnectTimeout(30);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds

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
    _settings.save();
    ESP.restart();
  }

  mqttclient.setServer(_settings.data.mqttServer, _settings.data.mqttPort);
  DEBUG_PRINTLN(F("MQTT Server config Loaded"));

  mqttclient.setCallback(mqttcallback);
  // mqttclient.setBufferSize(MQTT_BUFFER);
  //  check is WiFi connected
  if (!res)
  {
    DEBUG_PRINTLN(F("Failed to connect to WiFi or hit timeout"));
  }
  else
  {
    deviceJson["IP"] = WiFi.localIP(); // grab the device ip

    bms.Init(); // init the bms driver
    bms.callback(prozessUartData);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_MAIN, htmlProcessor);
request->send(response); });

    server.on("/livejson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                serializeJson(bmsJson, *response);
                request->send(response); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device reboots...");
                response->addHeader("Refresh", "3; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                RestartTimer = millis();
                restartNow = true; });

    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_CONFIRM_RESET, htmlProcessor);
request->send(response); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device is Erasing...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                delay(1000);
                _settings.reset();
                ESP.eraseConfig();
                ESP.restart(); });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS, htmlProcessor);
request->send(response); });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS_EDIT, htmlProcessor);
request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                strncpy(_settings.data.mqttServer, request->arg("post_mqttServer").c_str(), 40);
                _settings.data.mqttPort = request->arg("post_mqttPort").toInt();
                strncpy(_settings.data.mqttUser, request->arg("post_mqttUser").c_str(), 40);
                strncpy(_settings.data.mqttPassword, request->arg("post_mqttPassword").c_str(), 40);
                strncpy(_settings.data.mqttTopic, request->arg("post_mqttTopic").c_str(), 40);
                _settings.data.mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt(); // prevent lower numbers
                strncpy(_settings.data.deviceName, request->arg("post_deviceName").c_str(), 40);

                _settings.data.mqttJson = (request->arg("post_mqttjson") == "true") ? true : false;
                _settings.data.wakeupEnable = (request->arg("post_wakeupenable") == "true") ? true : false;
                _settings.data.relaisEnable = (request->arg("post_relaisenable") == "true") ? true : false;
                _settings.data.relaisInvert = (request->arg("post_relaisinvert") == "true") ? true : false;
                
                _settings.data.relaisFailsafe = (request->arg("post_relaisfailsafe") == "true") ? true : false;
                  
                _settings.data.relaisFunction = request->arg("post_relaisfunction").toInt();
                _settings.data.relaisComparsion = request->arg("post_relaiscomparsion").toInt();
                _settings.data.relaisSetValue = request->arg("post_relaissetvalue").toFloat();
                _settings.data.relaisHysteresis = request->arg("post_relaishysteresis").toFloat();
                _settings.save();
                request->redirect("/reboot"); });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebParameter *p = request->getParam(0);
                if (p->name() == "chargefet")
                {
                    DEBUG_PRINTLN(F("Webcall: charge fet to: ")+(String)p->value());
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
                    DEBUG_PRINTLN(F("Webcall: discharge fet to: ")+(String)p->value());
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
                    DEBUG_PRINTLN(F("Webcall: setsoc SOC set to: ")+(String)p->value());
                    if(p->value().toInt() >= 0 && p->value().toInt() <= 100 ){
                      bms.setSOC(p->value().toInt());
                    }
                }
                if (p->name() == "relais")
                {
                    DEBUG_PRINTLN(F("Webcall: set relais to: ")+(String)p->value());
                    if(p->value().toInt() == 1){
                      relaisComparsionResult = true;
                    }
                    if(p->value().toInt() == 0){
                      relaisComparsionResult = false;
                    }
                }
                request->send(200, "text/plain", "message received"); });

    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
          Serial.end();
          updateProgress = true;
          ws.enable(false);
          ws.closeAll();
          request->send(200); },
        handle_update_progress_cb);

    // set the device name
    MDNS.addService("http", "tcp", 80);
    if (MDNS.begin(_settings.data.deviceName))
      DEBUG_PRINTLN(F("mDNS running..."));
    WiFi.hostname(_settings.data.deviceName);
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();

    DEBUG_PRINTLN(F("Webserver Running..."));
    connectMQTT();
  }
}
// end void setup

void loop()
{
  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  {
    ws.cleanupClients(); // clean unused client connections
    MDNS.update();
    mqttclient.loop(); // Check if we have something to read from MQTT

    if (!updateProgress)
    {
      if (millis() >= (bmstimer + (3 * 1000)) && wsClient != nullptr && wsClient->canSend())
      {
        getJsonDevice();
        bms.update();
        if (bms.getState() >= 0) // check bms connection
        {
          getJsonData();
          notifyClients();
          bmstimer = millis();
        }
        else if (bms.getState() == -2)
        {
          getJsonData();
          notifyClients();
          //packJson[F("Status")] = "offline";
          bmstimer = millis();
        }
      }
      if (millis() >= (mqtttimer + (_settings.data.mqttRefresh * 1000)))
      {
        if (millis() <= (bmstimer + (3 * 1000))) // if the last request shorter then 3 use the data from last web request
        {
          getJsonData();
          sendtoMQTT();
          mqtttimer = millis();
        }
        else // get new data
        {
          getJsonDevice();
          bms.update();
          if (bms.getState() >= 0)
          {
            getJsonData();
            sendtoMQTT();
            mqtttimer = millis();
          }
          else if (bms.getState() == -2)
          {
            getJsonData();
            sendtoMQTT();
            //packJson[F("Status")] = "offline";
            mqtttimer = millis();
          }
        }
      }
    }
  }
  if (restartNow && millis() >= (RestartTimer + 500))
  {
    DEBUG_PRINTLN(F("Restart"));
    ESP.restart();
  }
  wakeupHandler();
  relaisHandler();

  notificationLED(); // notification LED routine
}
// End void loop

void prozessUartData()
{
  if (!updateProgress)
  {
    /*
    DEBUG_PRINTLN(F("Hello world as callback from uart!!!!!!!!!!!!!!!!!"));
    getJsonDevice();

    getJsonData();

    notifyClients();

    if (millis() > (mqtttimer + (_settings.data.mqttRefresh * 1000)))
    {
        sendtoMQTT();
        mqtttimer = millis();
    }
    */
  }
}

void getJsonDevice()
{
  deviceJson[F("ESP_VCC")] = ESP.getVcc() / 1000.0;
  deviceJson[F("Relais_Active")] = relaisComparsionResult ? true : false;
  deviceJson[F("Relais_Manual")] = _settings.data.relaisEnable && _settings.data.relaisFunction == 4 ? true : false;
#ifdef DALY_BMS_DEBUG
  deviceJson[F("Free_Heap")] = ESP.getFreeHeap();
  deviceJson[F("json_memory_usage")] = bmsJson.memoryUsage();
  deviceJson[F("json_capacity")] = bmsJson.capacity();
  deviceJson[F("runtime")] = millis() / 1000;
  deviceJson[F("ws_clients")] = ws.count();
  deviceJson[F("HEAP_Fragmentation")] = ESP.getHeapFragmentation();
  deviceJson[F("free_blocksize")] = ESP.getMaxFreeBlockSize();
#endif
}

void getJsonData()
{
  packJson[F("Voltage")] = bms.get.packVoltage;
  packJson[F("Current")] = bms.get.packCurrent;
  packJson[F("Power")] = (bms.get.packCurrent * bms.get.packVoltage);
  packJson[F("SOC")] = bms.get.packSOC;
  packJson[F("Remaining_mAh")] = bms.get.resCapacitymAh;
  packJson[F("Cycles")] = bms.get.bmsCycles;
  packJson[F("BMS_Temp")] = bms.get.tempAverage;
  packJson[F("Cell_Temp")] = bms.get.cellTemperature[0];
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

bool sendtoMQTT()
{
  char msgBuffer[32];
  if (!connectMQTT())
  {
    DEBUG_PRINTLN(F("Error: No connection to MQTT Server, cant send Data!"));
    firstPublish = false;
    return false;
  }
  DEBUG_PRINT(F("Info: Data sent to MQTT Server... "));
  if (!_settings.data.mqttJson)
  {

    mqttclient.publish(String(topicStrg + "/Pack_Voltage").c_str(), (const char *)dtostrf(bms.get.packVoltage, 4, 1, msgBuffer));
    mqttclient.publish(String(topicStrg + "/Pack_Current").c_str(), (const char *)dtostrf(bms.get.packCurrent, 4, 1, msgBuffer));
    mqttclient.publish(String(topicStrg + "/Pack_Power").c_str(), (const char *)dtostrf((bms.get.packVoltage * bms.get.packCurrent), 4, 1, msgBuffer));
    mqttclient.publish(String(topicStrg + "/Pack_SOC").c_str(), (const char *)dtostrf(bms.get.packSOC, 6, 2, msgBuffer));
    mqttclient.publish(String(topicStrg + "/Pack_Remaining_mAh").c_str(), (const char *)itoa(bms.get.resCapacitymAh, msgBuffer, 10));
    mqttclient.publish(String(topicStrg + "/Pack_Cycles").c_str(), (const char *)itoa(bms.get.bmsCycles, msgBuffer, 10));
    mqttclient.publish(String(topicStrg + "/Pack_BMS_Temperature").c_str(), (const char *)itoa(bms.get.tempAverage, msgBuffer, 10));
    mqttclient.publish(String(topicStrg + "/Pack_High_Cell").c_str(), (const char *)(dtostrf(bms.get.maxCellVNum, 1, 0, msgBuffer) + String(".- ") + dtostrf(bms.get.maxCellmV / 1000, 5, 3, msgBuffer)).c_str());
    mqttclient.publish(String(topicStrg + "/Pack_Low_Cell").c_str(), (const char *)(dtostrf(bms.get.minCellVNum, 1, 0, msgBuffer) + String(".- ") + dtostrf(bms.get.minCellmV / 1000, 5, 3, msgBuffer)).c_str());
    mqttclient.publish(String(topicStrg + "/Pack_Cell_Difference").c_str(), (const char *)itoa(bms.get.cellDiff, msgBuffer, 10));
    mqttclient.publish(String(topicStrg + "/Pack_ChargeFET").c_str(), (const char *)bms.get.chargeFetState ? "true" : "false");
    mqttclient.publish(String(topicStrg + "/Pack_DischargeFET").c_str(), (const char *)bms.get.disChargeFetState ? "true" : "false");
    mqttclient.publish(String(topicStrg + "/Pack_Status").c_str(), (const char *)bms.get.chargeDischargeStatus);
    mqttclient.publish(String(topicStrg + "/Pack_Cells").c_str(), (const char *)itoa(bms.get.numberOfCells, msgBuffer, 10));
    mqttclient.publish(String(topicStrg + "/Pack_Heartbeat").c_str(), (const char *)itoa(bms.get.bmsHeartBeat, msgBuffer, 10));
    mqttclient.publish(String(topicStrg + "/Pack_Balance_Active").c_str(), (const char *)bms.get.cellBalanceActive ? "true" : "false");
    mqttclient.loop();

    for (size_t i = 0; i < size_t(bms.get.numberOfCells); i++)
    {
      mqttclient.publish(String(topicStrg + "/Pack_Cells_Voltage/Cell_" + (i + 1)).c_str(), (const char *)dtostrf(bms.get.cellVmV[i] / 1000, 5, 3, msgBuffer));
      mqttclient.publish(String(topicStrg + "/Pack_Cells_Balance/Cell_" + (i + 1)).c_str(), (const char *)bms.get.cellBalanceState[i] ? "true" : "false");
      mqttclient.loop();
    }

    for (size_t i = 0; i < size_t(bms.get.numOfTempSensors); i++)
    {
      mqttclient.publish(String(topicStrg + "/Pack_Cell_Temperature_" + (i + 1)).c_str(), (const char *)itoa(bms.get.cellTemperature[i], msgBuffer, 10));
    }
  }
  else
  {
    char data[JSON_BUFFER];
    size_t len = serializeJson(bmsJson, data);
    mqttclient.setBufferSize(JSON_BUFFER+100);
    mqttclient.publish(String(topicStrg + "/Pack_Data").c_str(), data, len);
  }
  mqttclient.publish(String(topicStrg + "/RelaisOutput_Active").c_str(), (const char *)relaisComparsionResult ? "true" : "false");
  mqttclient.publish(String(topicStrg + "/RelaisOutput_Manual").c_str(), (const char *)(_settings.data.relaisFunction == 4) ? "true" : "false"); // should we keep this? you can check with iobroker etc. if you can even switch the relais using mqtt
  DEBUG_PRINT(F("Done\n"));
  firstPublish = true;

  return true;
}

void mqttcallback(char *topic, unsigned char *payload, unsigned int length)
{
  if (firstPublish == false)
    return;

  updateProgress = true;

  String messageTemp;
  char *top = topic;
  for (unsigned int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }

  // check if the message not empty
  if (messageTemp.length() <= 0)
  {
    DEBUG_PRINTLN(F("MQTT Callback: message empty, break!"));
    updateProgress = false;
    return;
  }

  if (!_settings.data.mqttJson)
  {
    DEBUG_PRINTLN(F("MQTT Callback: message recived: ") + messageTemp);
    // set Relais
    if (strcmp(top, (topicStrg + "/Device_Control/Relais").c_str()) == 0)
    {
      if (_settings.data.relaisFunction == 4 && messageTemp == "true")
      {
        DEBUG_PRINTLN(F("MQTT Callback: switching Relais on"));
        relaisComparsionResult = true;
        mqttclient.publish(String(topicStrg + "/Device_Control/Relais").c_str(), "true", false);
        relaisHandler();
      }
      if (_settings.data.relaisFunction == 4 && messageTemp == "false")
      {
        DEBUG_PRINTLN(F("MQTT Callback: switching Relais off"));
        relaisComparsionResult = false;
        mqttclient.publish(String(topicStrg + "/Device_Control/Relais").c_str(), "false", false);
        relaisHandler();
      }
    }
    // set SOC
    if (strcmp(top, (topicStrg + "/Device_Control/Pack_SOC").c_str()) == 0)
    {
      if (bms.get.packSOC != atof(messageTemp.c_str()) && atof(messageTemp.c_str()) >= 0 && atof(messageTemp.c_str()) <= 100)
      {
        if (bms.setSOC(atof(messageTemp.c_str())))
        {
          DEBUG_PRINTLN(F("MQTT Callback: SOC message OK, Write: ") + messageTemp);
          mqttclient.publish(String(topicStrg + "/Device_Control/Pack_SOC").c_str(), String(atof(messageTemp.c_str())).c_str(), false);
        }
      }
    }

    // Switch the Discharging port
    if (strcmp(top, (topicStrg + "/Device_Control/Pack_DischargeFET").c_str()) == 0)
    {
      if (messageTemp == "true" && !bms.get.disChargeFetState)
      {
        DEBUG_PRINTLN(F("MQTT Callback: switching Discharging mos on"));
        bms.setDischargeMOS(true);
        mqttclient.publish(String(topicStrg + "/Device_Control/Pack_DischargeFET").c_str(), "true", false);
      }
      if (messageTemp == "false" && bms.get.disChargeFetState)
      {
        DEBUG_PRINTLN(F("MQTT Callback: switching Discharging mos off"));
        bms.setDischargeMOS(false);
        mqttclient.publish(String(topicStrg + "/Device_Control/Pack_DischargeFET").c_str(), "false", false);
      }
    }

    // Switch the Charging Port
    if (strcmp(top, (topicStrg + "/Device_Control/Pack_ChargeFET").c_str()) == 0)
    {
      DEBUG_PRINTLN(F("message recived: ") + messageTemp);

      if (messageTemp == "true" && !bms.get.chargeFetState)
      {
        DEBUG_PRINTLN(F("MQTT Callback: switching Charging mos on"));
        bms.setChargeMOS(true);
        mqttclient.publish(String(topicStrg + "/Device_Control/Pack_ChargeFET").c_str(), "true", false);
      }
      if (messageTemp == "false" && bms.get.chargeFetState)
      {
        DEBUG_PRINTLN(F("MQTT Callback: switching Charging mos off"));
        bms.setChargeMOS(false);
        mqttclient.publish(String(topicStrg + "/Device_Control/Pack_ChargeFET").c_str(), "false", false);
      }
    }
  }
  else
  {
    StaticJsonDocument<JSON_BUFFER> mqttJsonAnswer;
    // DynamicJsonDocument mqttJsonAnswer(JSON_BUFFER);
    deserializeJson(mqttJsonAnswer, (const byte *)payload, length);
    bms.setChargeMOS(mqttJsonAnswer["Pack"]["SOC"]);

    if (mqttJsonAnswer["Pack"]["ChargeFET"] == true)
    {
      bms.setChargeMOS(true);
    }
    else if (mqttJsonAnswer["Pack"]["ChargeFET"] == false)
    {
      bms.setChargeMOS(false);
    }
    else
    {
      DEBUG_PRINTLN(F("No Valid Command from JSON for setChargeMOS"));
    }
    if (mqttJsonAnswer["Pack"]["DischargeFET"] == true)
    {
      bms.setDischargeMOS(true);
    }
    else if (mqttJsonAnswer["Pack"]["DischargeFET"] == false)
    {
      bms.setDischargeMOS(false);
    }
    else
    {
      DEBUG_PRINTLN(F("No Valid Command from JSON for setDischargeMOS"));
    }
  }
  updateProgress = false;
}

bool connectMQTT()
{
  if (!mqttclient.connected())
  {
    firstPublish = false;
    DEBUG_PRINT(F("Info: MQTT Client State is: "));
    DEBUG_PRINTLN(mqttclient.state());
    DEBUG_PRINT(F("Info: establish MQTT Connection... "));

    if (mqttclient.connect(mqttClientId, _settings.data.mqttUser, _settings.data.mqttPassword, (topicStrg + "/alive").c_str(), 0, true, "false", true))
    {
      if (mqttclient.connected())
      {
        DEBUG_PRINT(F("Done\n"));
        mqttclient.publish(String(topicStrg + "/alive").c_str(), "true", true); // LWT online message must be retained!
        mqttclient.publish((topicStrg + "/Device_IP").c_str(), (WiFi.localIP().toString()).c_str());
        if (!_settings.data.mqttJson)
        {
          mqttclient.subscribe(String(topicStrg + "/Device_Control/Pack_DischargeFET").c_str());
          mqttclient.subscribe(String(topicStrg + "/Device_Control/Pack_ChargeFET").c_str());
          mqttclient.subscribe(String(topicStrg + "/Device_Control/Pack_SOC").c_str());
          if (_settings.data.relaisFunction == 4)
            mqttclient.subscribe(String(topicStrg + "/Device_Control/Relais").c_str());
        }
        else
        {
          mqttclient.subscribe(String(topicStrg).c_str());
        }
      }
      else
      {
        DEBUG_PRINT(F("Fail\n"));
      }
    }
    else
    {
      DEBUG_PRINT(F("Fail\n"));
      return false; // Exit if we couldnt connect to MQTT brooker
    }
    firstPublish = true;
  }
  return true;
}
