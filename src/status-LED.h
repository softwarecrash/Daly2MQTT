#include <Arduino.h>
/***
https://wokwi.com/projects/357439918883980289
*/
extern Daly_BMS_UART bms;
unsigned int ledPin = 0;
unsigned int ledTimer = 0;
unsigned int repeatTime = 5000;
unsigned int cycleTime = 250;
unsigned int cycleMillis = 0;
byte ledState = 0;

bool waveHelper = false;

bool relaisOn = false;

void notificationLED()
{
  /*
  if (millis() >= (ledTimer + repeatTime) && ledState == 0)
  {
    if (true)
      ledState = 4;
    else if (false)
      ledState = 3;
    else if (false)
      ledState = 2;
    else if (false)
      ledState = 1;
  }

  if (ledState > 0)
  {
    if (millis() >= (cycleMillis + cycleTime) && relaisOn != true)
    {
      if (ledPin == 0)
      {
        ledPin = 255;
        ledState--;
      }
      else
      {
        ledPin = 0;
      }
      cycleMillis = millis();
      if (ledState == 0)
      {
        ledTimer = millis();
      }
    }

    if (millis() >= (cycleMillis + cycleTime) && relaisOn == true)
    {
      // other testing https://solarianprogrammer.com/2017/01/21/arduino-pulse-led-cosine-wave-function/
      // ledPin = 127.0 + 127.0 * sin((millis() / (float)(cycleTime * 2)) * 2.0 * PI);

      // ledPin = (cos((millis() / (float)(cycleTime/2)) - PI)*0.5+0.5)*255;

      if (ledPin > 250 && waveHelper == false)
      {
        ledState--;
        waveHelper = true;
      }
      else if (ledPin <= 1)
      {
        waveHelper = false;
        if (ledState == 0)
        {
          ledPin = 0;
        }
      }
      ledPin = (cos((millis() / (float)(cycleTime / 4)) - PI) * 0.5 + 0.5) * 255;

      if (ledState == 0)
      {
        ledTimer = millis();
      }
    }
  }

  analogWrite(LED_PIN, ledPin);
  */
}