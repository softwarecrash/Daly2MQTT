/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

#include <Arduino.h>

// json crack: https://jsoncrack.com/editor
#include <daly-bms-uart.h> // This is where the library gets pulled in
#define BMS_SERIAL Serial  // Set the serial port for communication with the Daly BMS
// #define DALY_BMS_DEBUG Serial1 // Uncomment the below #define to enable debugging print statements.

#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Settings.h"

#include "webpages/htmlCase.h"     // The HTML Konstructor
#include "webpages/main.h"         // landing page with menu
#include "webpages/settings.h"     // settings page
#include "webpages/settingsedit.h" // mqtt settings page

WiFiClient client;
Settings _settings;
PubSubClient mqttclient(client);
int jsonBufferSize = 2048;
char jsonBuffer[2048];

DynamicJsonDocument bmsJson(jsonBufferSize);                      // main Json
JsonObject packJson = bmsJson.createNestedObject("Pack");         // battery package data
JsonObject cellVJson = bmsJson.createNestedObject("CellV");       // nested data for cell voltages
JsonObject cellTempJson = bmsJson.createNestedObject("CellTemp"); // nested data for cell temp

String topicStrg;

unsigned long mqtttimer = 0;
unsigned long bmstimer = 0;
unsigned long RestartTimer = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;
Daly_BMS_UART bms(BMS_SERIAL);

// flag for saving data and other things
bool shouldSaveConfig = false;
char mqtt_server[40];
bool restartNow = false;
bool updateProgress = false;
bool dataCollect = false;
int crcErrCount = 0;
bool firstPublish = false;

//----------------------------------------------------------------------
void saveConfigCallback()
{
  Serial.println(F("Should save config"));
  shouldSaveConfig = true;
}

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{

  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!index)
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Starting Firmware Update"));
#endif
    Update.runAsync(true);
    if (!Update.begin(free_space, U_FLASH))
    {
#ifdef DALY_BMS_DEBUG
      Update.printError(DALY_BMS_DEBUG);
#endif
      ESP.restart();
    }
  }

  if (Update.write(data, len) != len)
  {
#ifdef DALY_BMS_DEBUG
    Update.printError(DALY_BMS_DEBUG);
#endif
    ESP.restart();
  }

  if (final)
  {
    if (!Update.end(true))
    {
#ifdef DALY_BMS_DEBUG
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

#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("Update complete");
#endif
      RestartTimer = millis();
      restartNow = true; // Set flag so main loop can issue restart call
    }
  }
}

void notifyClients()
{
  if (wsClient != nullptr && wsClient->canSend())
  {
    serializeJson(bmsJson, jsonBuffer);
    wsClient->text(jsonBuffer);
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
    delay(200); // give the bms time to react
    updateProgress = false;
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    wsClient = client;
    bms.update();
    getJsonData();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    wsClient = nullptr;
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
  wifi_set_sleep_type(LIGHT_SLEEP_T);
#ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.begin(9600); // Debugging towards UART1
#endif

  _settings.load();
  bms.Init();                                      // init the bms driver
  WiFi.persistent(true);                           // fix wifi save bug
  packJson["Device_Name"] = _settings._deviceName; // set the device name in json string
  topicStrg = (_settings._mqttTopic + "/" + _settings._deviceName).c_str();
  AsyncWiFiManager wm(&server, &dns);
  wm.setDebugOutput(false); // disable wifimanager debug output
  bmstimer = millis();
  mqtttimer = millis();
  wm.setSaveConfigCallback(saveConfigCallback);

#ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.println();
  DALY_BMS_DEBUG.print(F("Device Name:\t"));
  DALY_BMS_DEBUG.println(_settings._deviceName);
  DALY_BMS_DEBUG.print(F("Mqtt Server:\t"));
  DALY_BMS_DEBUG.println(_settings._mqttServer);
  DALY_BMS_DEBUG.print(F("Mqtt Port:\t"));
  DALY_BMS_DEBUG.println(_settings._mqttPort);
  DALY_BMS_DEBUG.print(F("Mqtt User:\t"));
  DALY_BMS_DEBUG.println(_settings._mqttUser);
  DALY_BMS_DEBUG.print(F("Mqtt Passwort:\t"));
  DALY_BMS_DEBUG.println(_settings._mqttPassword);
  DALY_BMS_DEBUG.print(F("Mqtt Interval:\t"));
  DALY_BMS_DEBUG.println(_settings._mqttRefresh);
  DALY_BMS_DEBUG.print(F("Mqtt Topic:\t"));
  DALY_BMS_DEBUG.println(_settings._mqttTopic);
#endif
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
    _settings._mqttServer = custom_mqtt_server.getValue();
    _settings._mqttUser = custom_mqtt_user.getValue();
    _settings._mqttPassword = custom_mqtt_pass.getValue();
    _settings._mqttPort = atoi(custom_mqtt_port.getValue());
    _settings._deviceName = custom_device_name.getValue();
    _settings._mqttTopic = custom_mqtt_topic.getValue();
    _settings._mqttRefresh = atoi(custom_mqtt_refresh.getValue());

    _settings.save();
    delay(500);
    //_settings.load();
    ESP.restart();
  }

  mqttclient.setServer(_settings._mqttServer.c_str(), _settings._mqttPort);
  mqttclient.setCallback(callback);
  mqttclient.setBufferSize(jsonBufferSize);
  // check is WiFi connected
  if (!res)
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Failed to connect to WiFi or hit timeout"));
#endif
  }
  else
  {

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_MAIN);
                response->printf_P(HTML_FOOT);
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
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_CONFIRM_RESET);
                response->printf_P(HTML_FOOT);
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
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS_EDIT);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/settingsjson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument SettingsJson(256);
                SettingsJson["device_name"] = _settings._deviceName;
                SettingsJson["mqtt_server"] = _settings._mqttServer;
                SettingsJson["mqtt_port"] = _settings._mqttPort;
                SettingsJson["mqtt_topic"] = _settings._mqttTopic;
                SettingsJson["mqtt_user"] = _settings._mqttUser;
                SettingsJson["mqtt_password"] = _settings._mqttPassword;
                SettingsJson["mqtt_refresh"] = _settings._mqttRefresh;
                SettingsJson["mqtt_json"] = _settings._mqttJson?true:false;
                serializeJson(SettingsJson, *response);
                request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                _settings._mqttServer = request->arg("post_mqttServer");
                _settings._mqttPort = request->arg("post_mqttPort").toInt();
                _settings._mqttUser = request->arg("post_mqttUser");
                _settings._mqttPassword = request->arg("post_mqttPassword");
                _settings._mqttTopic = request->arg("post_mqttTopic");
                _settings._mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt(); // prefent lower numbers
                _settings._deviceName = request->arg("post_deviceName");
                if (request->arg("post_mqttjson") == "true")
                  _settings._mqttJson = true;
                if (request->arg("post_mqttjson") != "true")
                  _settings._mqttJson = false;
                Serial.print(_settings._mqttServer);
                _settings.save();
                request->redirect("/reboot"); });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebParameter *p = request->getParam(0);
                if (p->name() == "chargefet")
                {
#ifdef DALY_BMS_DEBUG
                    DALY_BMS_DEBUG.println("Webcall: charge fet to: "+(String)p->value());
#endif
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
#ifdef DALY_BMS_DEBUG
                    DALY_BMS_DEBUG.println("Webcall: discharge fet to: "+(String)p->value());
#endif
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
#ifdef DALY_BMS_DEBUG
                    DALY_BMS_DEBUG.println("Webcall: setsoc SOC set to: "+(String)p->value());
#endif
                    if(p->value().toInt() >= 0 && p->value().toInt() <= 100 ){
                      bms.setSOC(p->value().toInt());
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
          request->send(200);
          // request->redirect("/");
        },
        handle_update_progress_cb);

    // set the device name
    MDNS.begin(_settings._deviceName);
    WiFi.hostname(_settings._deviceName);
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
    MDNS.addService("http", "tcp", 80);
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Webserver Running..."));
#endif
  }
  connectMQTT();
}
// end void setup

void loop()
{
  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  {                      // No use going to next step unless WIFI is up and running.
    ws.cleanupClients(); // clean unused client connections
    MDNS.update();
    mqttclient.loop(); // Check if we have something to read from MQTT

    if (!updateProgress)
    {
      bms.update();

      bool updatedData = false;
      if (millis() > (bmstimer + (5 * 1000)) && wsClient != nullptr && wsClient->canSend())
      {
        bmstimer = millis();
        if (bms.getState() >= 0) // ask the bms for new data
        {
          getJsonData();
          crcErrCount = 0;
          updatedData = true;
#ifdef DALY_BMS_DEBUG
          DALY_BMS_DEBUG.println(ESP.getFreeHeap(), DEC);
#endif
        }
        else
        {
          crcErrCount++;
          if (crcErrCount >= 3)
          {
            clearJsonData(); // by no connection, clear all data
            updatedData = false;
          }
        }
        notifyClients();
      }
      else if (millis() > (mqtttimer + (_settings._mqttRefresh * 1000)))
      {
        mqtttimer = millis();
        if (millis() < (bmstimer + (5 * 1000)) && updatedData == true) // if the last request shorter then 3 use the data from last web request
        {
          sendtoMQTT(); // Update data to MQTT server if we should
        }
        else // get new data
        {
          if (bms.getState() >= 0) // ask the bms for new data
          {
            getJsonData(); // prepare data for json string sending
            sendtoMQTT();  // Update data to MQTT server if we should
            crcErrCount = 0;
            updatedData = true;
          }
          else
          {
            crcErrCount++;
            if (crcErrCount >= 3)
            {
              clearJsonData(); // by no connection, clear all data
              updatedData = false;
            }
          }
        }
      }
    }
    if (wsClient == nullptr)
    {
      delay(2); // for power saving test
    }
  }
  if (restartNow && millis() >= (RestartTimer + 500))
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Restart"));
#endif
    ESP.restart();
  }
  yield();
}
// End void loop
void getJsonData()
{
  // prevent buffer leak
  if (int(bmsJson.memoryUsage()) >= (jsonBufferSize - 8))
  {
    bmsJson.garbageCollect();
  }
  packJson["Device_IP"] = WiFi.localIP().toString();
  packJson["Voltage"] = bms.get.packVoltage;
  packJson["Current"] = bms.get.packCurrent;
  packJson["Power"] = bms.get.packCurrent * bms.get.packVoltage;
  packJson["SOC"] = bms.get.packSOC;
  packJson["Remaining_mAh"] = bms.get.resCapacitymAh;
  packJson["Cycles"] = bms.get.bmsCycles;
  packJson["BMS_Temp"] = bms.get.tempAverage;
  packJson["Cell_Temp"] = bms.get.cellTemperature[0];
  packJson["High_CellNr"] = bms.get.maxCellVNum;
  packJson["High_CellV"] = bms.get.maxCellmV / 1000;
  packJson["Low_CellNr"] = bms.get.minCellVNum;
  packJson["Low_CellV"] = bms.get.minCellmV / 1000;
  packJson["Cell_Diff"] = bms.get.cellDiff;
  packJson["DischargeFET"] = bms.get.disChargeFetState ? true : false;
  packJson["ChargeFET"] = bms.get.chargeFetState ? true : false;
  packJson["Status"] = bms.get.chargeDischargeStatus;
  packJson["Cells"] = bms.get.numberOfCells;
  packJson["Heartbeat"] = bms.get.bmsHeartBeat;
  packJson["Balance_Active"] = bms.get.cellBalanceActive ? true : false;

  for (size_t i = 0; i < size_t(bms.get.numberOfCells); i++)
  {
    cellVJson["CellV " + String(i + 1)] = bms.get.cellVmV[i] / 1000;
    cellVJson["Balance " + String(i + 1)] = bms.get.cellBalanceState[i];
  }

  for (size_t i = 0; i < size_t(bms.get.numOfTempSensors); i++)
  {
    cellTempJson["Cell_Temp" + String(i + 1)] = bms.get.cellTemperature[i];
  }
}

void clearJsonData()
{
  packJson["Voltage"] = nullptr;
  packJson["Current"] = nullptr;
  packJson["Power"] = nullptr;
  packJson["SOC"] = nullptr;
  packJson["Remaining_mAh"] = nullptr;
  packJson["Cycles"] = nullptr;
  packJson["BMS_Temp"] = nullptr;
  packJson["Cell_Temp"] = nullptr;
  packJson["High_CellNr"] = nullptr;
  packJson["High_CellV"] = nullptr;
  packJson["Low_CellNr"] = nullptr;
  packJson["Low_CellV"] = nullptr;
  packJson["Cell_Diff"] = nullptr;
  packJson["DischargeFET"] = nullptr;
  packJson["ChargeFET"] = nullptr;
  packJson["Status"] = nullptr;
  packJson["Cells"] = nullptr;
  packJson["Heartbeat"] = nullptr;
  packJson["Balance_Active"] = nullptr;
  cellVJson.clear();
  cellTempJson.clear();
}

bool sendtoMQTT()
{
  if (!connectMQTT())
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Error: No connection to MQTT Server, can´t send Data!"));
#endif
    firstPublish = false;
    return false;
  }

  if (!_settings._mqttJson)
  {
    mqttclient.publish((topicStrg + ("/Device_IP")).c_str(), (WiFi.localIP().toString()).c_str());
    char msgBuffer[20];
    mqttclient.publish((topicStrg + "/Pack_Voltage").c_str(), dtostrf(bms.get.packVoltage, 4, 1, msgBuffer));
    mqttclient.publish((topicStrg + "/Pack_Current").c_str(), dtostrf(bms.get.packCurrent, 4, 1, msgBuffer));
    mqttclient.publish((topicStrg + "/Pack_Power").c_str(), dtostrf((bms.get.packVoltage * bms.get.packCurrent), 4, 1, msgBuffer));
    mqttclient.publish((topicStrg + "/Pack_SOC").c_str(), dtostrf(bms.get.packSOC, 6, 2, msgBuffer));
    mqttclient.publish((topicStrg + "/Pack_Remaining_mAh").c_str(), String(bms.get.resCapacitymAh).c_str());
    mqttclient.publish((topicStrg + "/Pack_Cycles").c_str(), String(bms.get.bmsCycles).c_str());
    mqttclient.publish((topicStrg + "/Pack_BMS_Temperature").c_str(), String(bms.get.tempAverage).c_str());
    mqttclient.publish((topicStrg + "/Pack_High_Cell").c_str(), (dtostrf(bms.get.maxCellVNum, 1, 0, msgBuffer) + String(".- ") + dtostrf(bms.get.maxCellmV / 1000, 5, 3, msgBuffer)).c_str());
    mqttclient.publish((topicStrg + "/Pack_Low_Cell").c_str(), (dtostrf(bms.get.minCellVNum, 1, 0, msgBuffer) + String(".- ") + dtostrf(bms.get.minCellmV / 1000, 5, 3, msgBuffer)).c_str());
    mqttclient.publish((topicStrg + "/Pack_Cell_Difference").c_str(), String(bms.get.cellDiff).c_str());
    mqttclient.publish((topicStrg + "/Pack_ChargeFET").c_str(), bms.get.chargeFetState ? "true" : "false");
    mqttclient.publish((topicStrg + "/Pack_DischargeFET").c_str(), bms.get.disChargeFetState ? "true" : "false");
    mqttclient.publish((topicStrg + "/Pack_Status").c_str(), bms.get.chargeDischargeStatus.c_str());
    mqttclient.publish((topicStrg + "/Pack_Cells").c_str(), String(bms.get.numberOfCells).c_str());
    mqttclient.publish((topicStrg + "/Pack_Heartbeat").c_str(), String(bms.get.bmsHeartBeat).c_str());
    mqttclient.publish((topicStrg + "/Pack_Balance_Active").c_str(), String(bms.get.cellBalanceActive ? "true" : "false").c_str());

    for (size_t i = 0; i < size_t(bms.get.numberOfCells); i++)
    {
      mqttclient.publish((topicStrg + "/Pack_Cells_Voltage/Cell_" + (i + 1)).c_str(), dtostrf(bms.get.cellVmV[i] / 1000, 5, 3, msgBuffer));
      mqttclient.publish((topicStrg + "/Pack_Cells_Balance/Cell_" + (i + 1)).c_str(), String(bms.get.cellBalanceState[i] ? "true" : "false").c_str());
    }

    for (size_t i = 0; i < size_t(bms.get.numOfTempSensors); i++)
    {
      mqttclient.publish((topicStrg + "/Pack_Cell_Temperature_" + (i + 1)).c_str(), String(bms.get.cellTemperature[i]).c_str());
    }
  }
  else
  {
    size_t n = serializeJson(bmsJson, jsonBuffer);
    mqttclient.publish((String(topicStrg)).c_str(), jsonBuffer, n);
  }
  firstPublish = true;
  return true;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  if (firstPublish == false)
    return;
  updateProgress = true;
  if (!_settings._mqttJson)
  {
    String messageTemp;
    char *top = topic;
    for (unsigned int i = 0; i < length; i++)
    {
      messageTemp += (char)payload[i];
    }
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println("message recived: " + messageTemp);
#endif

    // set SOC
    if (strcmp(top, (topicStrg + "/Pack_SOC").c_str()) == 0)
    {
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("message recived: " + messageTemp);
      DALY_BMS_DEBUG.println("set SOC");
#endif

      if (bms.get.packSOC != messageTemp.toInt())
      {
        bms.setSOC(messageTemp.toInt());
      }
    }

    // Switch the Discharging port
    if (strcmp(top, (topicStrg + "/Pack_DischargeFET").c_str()) == 0)
    {
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("message recived: " + messageTemp);
#endif

      if (messageTemp == "true" && !bms.get.disChargeFetState)
      {
#ifdef DALY_BMS_DEBUG
        DALY_BMS_DEBUG.println("switching Discharging mos on");
#endif
        bms.setDischargeMOS(true);
      }
      if (messageTemp == "false" && bms.get.disChargeFetState)
      {
#ifdef DALY_BMS_DEBUG
        DALY_BMS_DEBUG.println("switching Discharging mos off");
#endif
        bms.setDischargeMOS(false);
      }
    }

    // Switch the Charging Port
    if (strcmp(top, (topicStrg + "/Pack_ChargeFET").c_str()) == 0)
    {
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("message recived: " + messageTemp);
#endif

      if (messageTemp == "true" && !bms.get.chargeFetState)
      {
#ifdef DALY_BMS_DEBUG
        DALY_BMS_DEBUG.println("switching Charging mos on");
#endif
        bms.setChargeMOS(true);
      }
      if (messageTemp == "false" && bms.get.chargeFetState)
      {
#ifdef DALY_BMS_DEBUG
        DALY_BMS_DEBUG.println("switching Charging mos off");
#endif
        bms.setChargeMOS(false);
      }
    }
  }
  else
  {
    StaticJsonDocument<2048> mqttJsonAnswer;
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
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("No Valid Command from JSON for setChargeMOS");
#endif
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
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("No Valid Command from JSON for setDischargeMOS");
#endif
    }
  }
  updateProgress = false;
}

bool connectMQTT()
{
  if (!mqttclient.connected())
  {
#ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.print("Info: MQTT Client State is: ");
    DALY_BMS_DEBUG.println(mqttclient.state());
#endif
    if (mqttclient.connect(((_settings._deviceName)).c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str()))
    {
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println(F("Info: Connected to MQTT Server"));
#endif
      if (mqttclient.connect(_settings._deviceName.c_str()))
      {
        if (!_settings._mqttJson)
        {
          mqttclient.subscribe((topicStrg + "/Pack_DischargeFET").c_str());
          mqttclient.subscribe((topicStrg + "/Pack_ChargeFET").c_str());
          mqttclient.subscribe((topicStrg + "/Pack_SOC").c_str());
        }
        else
        {
          mqttclient.subscribe((topicStrg).c_str());
        }
      }
    }
    else
    {
#ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println(F("Error: No connection to MQTT Server"));
#endif
      return false; // Exit if we couldnt connect to MQTT brooker
    }
  }
#ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.println(F("Info: Data sent to MQTT Server"));
#endif
  return true;
}
