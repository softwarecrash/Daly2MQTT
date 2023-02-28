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
unsigned int ledPin = 0;
unsigned int ledTimer = 0;
unsigned int repeatTime = 5000;
unsigned int cycleTime = 250;
unsigned int cycleMillis = 0;
byte ledState = 0;

//bool waveHelper = false;
void notificationLED()
{

  if (millis() >= (ledTimer + repeatTime) && ledState == 0)
  {
    if (WiFi.status() != WL_CONNECTED)
      ledState = 4;
    else if (!mqttclient.connected())
      ledState = 3;
    else if (strcmp(bms.get.chargeDischargeStatus, "offline") == 0)
      ledState = 2;
    else if (WiFi.status() == WL_CONNECTED && mqttclient.connected() && strcmp(bms.get.chargeDischargeStatus, "offline") != 0)
      ledState = 1;
  }

  if (ledState > 0)
  {
    if (millis() >= (cycleMillis + cycleTime) /*&& relaisOn != true*/)
    {
      if (ledPin == 0)
      {
        ledPin = 255; 
      }
      else
      {
        ledPin = 0;
        ledState--;
      }
      cycleMillis = millis();
      if (ledState == 0)
      {
        ledTimer = millis();
      }
    }
/* make it later
    if (millis() >= (cycleMillis + cycleTime) && relaisOn == true)
    {
       //ledPin = 127.0 + 128.0 * sin((millis() / (float)(cycleTime * 2)) * 2.0 * PI);
       ledPin = (cos((millis() / (float)(cycleTime/4)) - PI)*0.5+0.5)*255;

      if (ledPin == 254 && waveHelper == false)
      {
        waveHelper = true;
      }
      if (ledPin == 0 && waveHelper == true)
      {
        ledState--;
        waveHelper = false;
      }


      if (ledState == 0)
      {
        ledTimer = millis();
      }
    }
    */
  }
  analogWrite(LED_PIN, 255 - ledPin);
}