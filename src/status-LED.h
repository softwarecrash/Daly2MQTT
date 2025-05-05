/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
#include <Arduino.h>
/*
  Blinking LED = Relais Off
  Waveing LED = Relais On
  every 5 seconds:
  1x all ok - Working
  2x no BMS Connection
  3x no MQTT Connection
  4x no WiFi Connection

*/
bool ledPin = 0;
unsigned int ledTimer = 0;
unsigned int repeatTime = 5000;
unsigned int cycleTime = 250;
unsigned int cycleMillis = 0;
byte ledState = 0;

void notificationLED()
{

  if (millis() >= (ledTimer + repeatTime) && ledState == 0)
  {
    if (WiFi.status() != WL_CONNECTED)
      ledState = 4;
    else if (!mqttclient.connected() && strlen(_settings.data.mqttServer) > 0)
      ledState = 3;
    else if (!bms.getState())
      ledState = 2;
    else if (WiFi.status() == WL_CONNECTED && strcmp(bms.get.chargeDischargeStatus, "offline") != 0)
      ledState = 1;
  }

  if (ledState > 0)
  {
    if (millis() >= (cycleMillis + cycleTime))
    {
      if (!ledPin)
      {
        ledPin = true; 
      }
      else
      {
        ledPin = false;
        ledState--;
      }
      cycleMillis = millis();
      if (ledState == 0)
      {
        ledTimer = millis();
      }
    }
  }
  if (!ledPin)
  {
    analogWrite(LED_PIN, 255);
  } else
  {
    analogWrite(LED_PIN, 255-_settings.data.LEDBrightness);
  }
}