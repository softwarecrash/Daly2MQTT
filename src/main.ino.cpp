# 1 "C:\\Users\\PCHILF~1\\AppData\\Local\\Temp\\tmpbqltanc3"
#include <Arduino.h>
# 1 "C:/Users/PC Hilfe Weimar/Documents/Git repos/DALY-BMS-to-MQTT/DALY-BMS-to-MQTT/src/main.ino"







#include <Arduino.h>


#include <daly-bms-uart.h>
#define BMS_SERIAL Serial


#define ARDUINOJSON_USE_DOUBLE 0

#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Settings.h"

#include "webpages/htmlCase.h"
#include "webpages/main.h"
#include "webpages/settings.h"
#include "webpages/settingsedit.h"

WiFiClient client;
Settings _settings;
PubSubClient mqttclient(client);
int jsonBufferSize = 2048;
char jsonBuffer[2048];

DynamicJsonDocument bmsJson(jsonBufferSize);
JsonObject packJson = bmsJson.createNestedObject("Pack");
JsonObject cellVJson = bmsJson.createNestedObject("CellV");
JsonObject cellTempJson = bmsJson.createNestedObject("CellTemp");

String topicStrg;

unsigned long mqtttimer = 0;
unsigned long bmstimer = 0;
unsigned long RestartTimer = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;
Daly_BMS_UART bms(BMS_SERIAL);


bool shouldSaveConfig = false;

bool restartNow = false;
bool updateProgress = false;
bool dataCollect = false;
bool firstPublish = false;

#define WAKEUP_PIN 12
#define WAKEUP_INTERVAL 10000
#define WAKEUP_DURATION 100
unsigned long wakeuptimer = WAKEUP_INTERVAL;
bool wakeupPinActive = false;


#define RELAISPIN 14
#define RELAISINTERVAL 5000

unsigned long relaistimer = RELAISINTERVAL;
float relaisCompareValueTmp = 0;
bool relaisComparsionResult = false;
void saveConfigCallback();
static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void notifyClients();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
bool wakeupHandler();
bool relaisHandler();
void setup();
void loop();
void getJsonData();
void clearJsonData();
bool sendtoMQTT();
void callback(char *topic, byte *payload, unsigned int length);
bool connectMQTT();
#line 76 "C:/Users/PC Hilfe Weimar/Documents/Git repos/DALY-BMS-to-MQTT/DALY-BMS-to-MQTT/src/main.ino"
void saveConfigCallback()
{
  #ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.println(F("Should save config"));
  #endif
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
      restartNow = true;
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
    if (strcmp((char *)data, "relaisOutputSwitch_on") == 0)
    {
      relaisComparsionResult = true;
      relaisHandler();
    }
    if (strcmp((char *)data, "relaisOutputSwitch_off") == 0)
    {
      relaisComparsionResult = false;
      relaisHandler();
    }
    delay(200);
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

bool wakeupHandler()
{
  if (_settings.data.wakeupEnable && (millis() > wakeuptimer))
  {
    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println();
    DALY_BMS_DEBUG.println("wakeupHandler()");
    DALY_BMS_DEBUG.print("this run:\t");
    DALY_BMS_DEBUG.println(millis());
    DALY_BMS_DEBUG.print("next run:\t");
    DALY_BMS_DEBUG.println(wakeuptimer);
    #endif
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
    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.print("PIN IS NOW:\t");
    DALY_BMS_DEBUG.println(digitalRead(WAKEUP_PIN));
    #endif
  }
  return true;
}

bool relaisHandler()
{
  if (_settings.data.relaisEnable && (millis() - relaistimer > RELAISINTERVAL))
  {
    relaistimer = millis();

    switch (_settings.data.relaisFunction)
    {
    case 0:

      relaisCompareValueTmp = bms.get.minCellmV / 1000;
      break;
    case 1:

      relaisCompareValueTmp = bms.get.maxCellmV / 1000;
      break;
    case 2:

      relaisCompareValueTmp = bms.get.packVoltage;
      break;
    case 3:

      relaisCompareValueTmp = bms.get.tempAverage;
      break;
    case 4:

      break;
    }

    if (relaisCompareValueTmp == '\0' && _settings.data.relaisFunction != 4)
    {
      return false;
    }

    if (_settings.data.relaisFunction != 4)
    {

      switch (_settings.data.relaisComparsion)
      {
      case 0:


        if (relaisComparsionResult)
        {
          relaisComparsionResult = relaisCompareValueTmp >= (_settings.data.relaisSetValue - _settings.data.relaisHysteresis) ? true : false;
        }
        else
        {

          relaisComparsionResult = relaisCompareValueTmp >= (_settings.data.relaisSetValue) ? true : false;
        }
        break;
      case 1:


        if (relaisComparsionResult)
        {

          relaisComparsionResult = relaisCompareValueTmp <= (_settings.data.relaisSetValue + _settings.data.relaisHysteresis) ? true : false;
        }
        else
        {

          relaisComparsionResult = relaisCompareValueTmp <= (_settings.data.relaisSetValue) ? true : false;
        }
        break;
      }
    }
    else
    {


    }

    _settings.data.relaisInvert ? digitalWrite(RELAISPIN, !relaisComparsionResult) : digitalWrite(RELAISPIN, relaisComparsionResult);

    return true;
  }
  return false;
}

void setup()
{
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  #ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.begin(9600);
  #endif
  _settings.load();
  if (_settings.data.wakeupEnable)
    pinMode(WAKEUP_PIN, OUTPUT);
  if (_settings.data.relaisEnable)
    pinMode(RELAISPIN, OUTPUT);
  bms.Init();
  WiFi.persistent(true);
  packJson["Device_Name"] = _settings.data.deviceName;
  topicStrg = (_settings.data.mqttTopic + String("/") + _settings.data.deviceName).c_str();
  AsyncWiFiManager wm(&server, &dns);
  wm.setDebugOutput(false);
  bmstimer = millis();
  mqtttimer = millis();
  wm.setSaveConfigCallback(saveConfigCallback);
#ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.println();
  DALY_BMS_DEBUG.print(F("Device Name:\t"));
  DALY_BMS_DEBUG.println(_settings.data.deviceName);
  DALY_BMS_DEBUG.print(F("Mqtt Server:\t"));
  DALY_BMS_DEBUG.println(_settings.data.mqttServer);
  DALY_BMS_DEBUG.print(F("Mqtt Port:\t"));
  DALY_BMS_DEBUG.println(_settings.data.mqttPort);
  DALY_BMS_DEBUG.print(F("Mqtt User:\t"));
  DALY_BMS_DEBUG.println(_settings.data.mqttUser);
  DALY_BMS_DEBUG.print(F("Mqtt Passwort:\t"));
  DALY_BMS_DEBUG.println(_settings.data.mqttPassword);
  DALY_BMS_DEBUG.print(F("Mqtt Interval:\t"));
  DALY_BMS_DEBUG.println(_settings.data.mqttRefresh);
  DALY_BMS_DEBUG.print(F("Mqtt Topic:\t"));
  DALY_BMS_DEBUG.println(_settings.data.mqttTopic);
  DALY_BMS_DEBUG.print(F("wakeupEnable:\t"));
  DALY_BMS_DEBUG.println(_settings.data.wakeupEnable);
  DALY_BMS_DEBUG.print(F("relaisEnable:\t"));
  DALY_BMS_DEBUG.println(_settings.data.relaisEnable);
  DALY_BMS_DEBUG.print(F("relaisInvert:\t"));
  DALY_BMS_DEBUG.println(_settings.data.relaisInvert);
  DALY_BMS_DEBUG.print(F("relaisFunction:\t"));
  DALY_BMS_DEBUG.println(_settings.data.relaisFunction);
  DALY_BMS_DEBUG.print(F("relaisComparsion:\t"));
  DALY_BMS_DEBUG.println(_settings.data.relaisComparsion);
  DALY_BMS_DEBUG.print(F("relaisSetValue:\t"));
  DALY_BMS_DEBUG.println(_settings.data.relaisSetValue);
  DALY_BMS_DEBUG.print(F("relaisHysteresis:\t"));
  DALY_BMS_DEBUG.println(_settings.data.relaisHysteresis);
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

  wm.setConnectTimeout(30);
  wm.setConfigPortalTimeout(120);


  if (shouldSaveConfig)
  {
    strcpy(_settings.data.mqttServer, custom_mqtt_server.getValue());
    strcpy(_settings.data.mqttUser, custom_mqtt_user.getValue());
    strcpy(_settings.data.mqttPassword, custom_mqtt_pass.getValue());
    _settings.data.mqttPort = atoi(custom_mqtt_port.getValue());
    strcpy(_settings.data.deviceName, custom_device_name.getValue());
    strcpy(_settings.data.mqttTopic, custom_mqtt_topic.getValue());
    _settings.data.mqttRefresh = atoi(custom_mqtt_refresh.getValue());

    _settings.save();
    delay(500);

    ESP.restart();
  }

  mqttclient.setServer(_settings.data.mqttServer, _settings.data.mqttPort);
  #ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.println("MQTT Server config Loaded");
  #endif

  mqttclient.setCallback(callback);
  mqttclient.setBufferSize(jsonBufferSize);

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
                DynamicJsonDocument SettingsJson(512);
                SettingsJson["device_name"] = _settings.data.deviceName;
                SettingsJson["mqtt_server"] = _settings.data.mqttServer;
                SettingsJson["mqtt_port"] = _settings.data.mqttPort;
                SettingsJson["mqtt_topic"] = _settings.data.mqttTopic;
                SettingsJson["mqtt_user"] = _settings.data.mqttUser;
                SettingsJson["mqtt_password"] = _settings.data.mqttPassword;
                SettingsJson["mqtt_refresh"] = _settings.data.mqttRefresh;
                SettingsJson["mqtt_json"] = _settings.data.mqttJson;
                SettingsJson["wakeup_enable"] = _settings.data.wakeupEnable;
                SettingsJson["relais_enable"] = _settings.data.relaisEnable;
                SettingsJson["relais_invert"] = _settings.data.relaisInvert;
                SettingsJson["relais_function"] = _settings.data.relaisFunction;
                SettingsJson["relais_comparsion"] = _settings.data.relaisComparsion;
                SettingsJson["relais_setvalue"] = _settings.data.relaisSetValue;
                SettingsJson["relais_hysteresis"] = _settings.data.relaisHysteresis;

                serializeJson(SettingsJson, *response);
                request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                strcpy(_settings.data.mqttServer, request->arg("post_mqttServer").c_str());
                _settings.data.mqttPort = request->arg("post_mqttPort").toInt();
                strcpy(_settings.data.mqttUser, request->arg("post_mqttUser").c_str());
                strcpy(_settings.data.mqttPassword, request->arg("post_mqttPassword").c_str());
                strcpy(_settings.data.mqttTopic, request->arg("post_mqttTopic").c_str());
                _settings.data.mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt();
                strcpy(_settings.data.deviceName, request->arg("post_deviceName").c_str());

                _settings.data.mqttJson = request->arg("post_mqttjson") == "true" ? true : false;
                _settings.data.wakeupEnable = request->arg("post_wakeupenable") == "true" ? true : false;
                _settings.data.relaisEnable = request->arg("post_relaisenable") == "true" ? true : false;
                _settings.data.relaisInvert = request->arg("post_relaisinvert") == "true" ? true : false;

                _settings.data.relaisFunction = request->arg("post_relaisfunction").toInt();
                _settings.data.relaisComparsion = request->arg("post_relaiscomparsion").toInt();
                _settings.data.relaisSetValue = request->arg("post_relaisSetValue").toFloat();
                _settings.data.relaisHysteresis = request->arg("post_relaishysteresis").toFloat();

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
          request->send(200); },
        handle_update_progress_cb);


    MDNS.begin(_settings.data.deviceName);
    WiFi.hostname(_settings.data.deviceName);
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Webserver Running..."));
    #endif
    connectMQTT();
  }

  mqtttimer = millis();
  bmstimer = millis();
}


void loop()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    ws.cleanupClients();
    MDNS.update();
    mqttclient.loop();

    if (!updateProgress)
    {
      if (millis() > (bmstimer + (3 * 1000)) && wsClient != nullptr && wsClient->canSend())
      {
        bms.update();
        if (bms.getState() >= 0)
        {
          bmstimer = millis();
          getJsonData();
        }
        if (bms.getState() <= -2)
        {
          clearJsonData();
        }
        notifyClients();
      }

      if (millis() > (mqtttimer + (_settings.data.mqttRefresh * 1000)))
      {
        if (millis() <= (bmstimer + (3 * 1000)))
        {
          sendtoMQTT();
          mqtttimer = millis();
        }
        else
        {
          bms.update();
          if (bms.getState() >= 0)
          {
            mqtttimer = millis();
            getJsonData();
            sendtoMQTT();
          }
          if (bms.getState() <= -2)
          {
            clearJsonData();
          }
        }
      }
    }
    if (wsClient == nullptr)
    {
      delay(2);
    }
  }
  if (restartNow && millis() >= (RestartTimer + 500))
  {
    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Restart"));
    #endif
    ESP.restart();
  }
  wakeupHandler();
  relaisHandler();

  yield();
}

void getJsonData()
{

  if (int(bmsJson.memoryUsage()) >= (jsonBufferSize - 8))
  {
    bmsJson.garbageCollect();
  }
  packJson["Device_IP"] = WiFi.localIP().toString();
  packJson["Voltage"] = bms.get.packVoltage;
  packJson["Current"] = bms.get.packCurrent;
  packJson["Power"] = (bms.get.packCurrent * bms.get.packVoltage);
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
  packJson["Relais_Active"] = relaisComparsionResult ? true : false;
  packJson["Relais_Manual"] = _settings.data.relaisEnable && _settings.data.relaisFunction == 4 ? true : false;

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
  packJson["Relais_Active"] = nullptr;
  packJson["Relais_Manual"] = nullptr;
  cellVJson.clear();
  cellTempJson.clear();
}

bool sendtoMQTT()
{
  if (!connectMQTT())
  {
    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println(F("Error: No connection to MQTT Server, cant send Data!"));
    #endif
    firstPublish = false;
    return false;
  }

  if (!_settings.data.mqttJson)
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

  mqttclient.publish((topicStrg + "/RelaisOutput_Active").c_str(), String(relaisComparsionResult ? "true" : "false").c_str());
  mqttclient.publish((topicStrg + "/RelaisOutput_Manual").c_str(), String(_settings.data.relaisFunction == 4 ? "true" : "false").c_str());

  firstPublish = true;
  return true;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  if (firstPublish == false)
    return;
  updateProgress = true;
  if (!_settings.data.mqttJson)
  {
    String messageTemp;
    char *top = topic;
    for (unsigned int i = 0; i < length; i++)
    {
      messageTemp += (char)payload[i];
    }


    if(messageTemp.length() <= 0)
    {
    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println("Callback message empty, break");
    #endif
    return;
    }

    #ifdef DALY_BMS_DEBUG
    DALY_BMS_DEBUG.println("message recived: " + messageTemp);
    #endif


    if (strcmp(top, (topicStrg + "/Device_Control/Relais").c_str()) == 0)
    {
      #ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("message recived: " + messageTemp);
      DALY_BMS_DEBUG.println("set Relais");
      #endif
      if (_settings.data.relaisFunction == 4 && messageTemp == "true")
      {
        #ifdef DALY_BMS_DEBUG
        DALY_BMS_DEBUG.println("switching Relais on");
        #endif
        relaisComparsionResult = true;
        relaisHandler();
      }
      if (_settings.data.relaisFunction == 4 && messageTemp == "false")
      {
        #ifdef DALY_BMS_DEBUG
        DALY_BMS_DEBUG.println("switching Relais off");
        #endif
        relaisComparsionResult = false;
        relaisHandler();
      }
    }


    if (strcmp(top, (topicStrg + "/Device_Control/Pack_SOC").c_str()) == 0)
    {
      #ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("message recived: " + messageTemp);
      #endif
      if (bms.get.packSOC != atoi(messageTemp.c_str()) && atoi(messageTemp.c_str()) >=0 && atoi(messageTemp.c_str()) <= 100)
      {
      #ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println("SOC message OK, Write: " + messageTemp);
      DALY_BMS_DEBUG.println("set SOC");
      #endif
        bms.setSOC(atoi(messageTemp.c_str()));
      }
    }


    if (strcmp(top, (topicStrg + "/Device_Control/Pack_DischargeFET").c_str()) == 0)
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


    if (strcmp(top, (topicStrg + "/Device_Control/Pack_ChargeFET").c_str()) == 0)
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
    StaticJsonDocument<1024> mqttJsonAnswer;
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
    if (mqttclient.connect(_settings.data.deviceName, _settings.data.mqttUser, _settings.data.mqttPassword))
    {
      #ifdef DALY_BMS_DEBUG
      DALY_BMS_DEBUG.println(F("Info: Connected to MQTT Server"));
      #endif
      if (mqttclient.connect(_settings.data.deviceName))
      {
        if (!_settings.data.mqttJson)
        {
          mqttclient.subscribe((topicStrg + "/Device_Control/Pack_DischargeFET").c_str());
          mqttclient.subscribe((topicStrg + "/Device_Control/Pack_ChargeFET").c_str());
          mqttclient.subscribe((topicStrg + "/Device_Control/Pack_SOC").c_str());
          if (_settings.data.relaisFunction == 4)
            mqttclient.subscribe((topicStrg + "/Device_Control/Relais").c_str());
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
      return false;
    }
  }
  #ifdef DALY_BMS_DEBUG
  DALY_BMS_DEBUG.println(F("Info: Data sent to MQTT Server"));
  #endif
  return true;
}