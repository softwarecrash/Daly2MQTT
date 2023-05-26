#include <Arduino.h>

int relaisState;
unsigned long previousMillis = 0;
const long timeout = 10000;
const int signalInput = 2;
const int signalOutput = 3;
enum states
{
  WATCH,
  WAIT
};
unsigned int state;
void setup()
{
  state = WATCH;
  pinMode(signalInput, INPUT);
  pinMode(signalOutput, OUTPUT);
  delay(2000);
  relaisState = digitalRead(signalInput);
}

void loop()
{
  unsigned long currentMillis = millis();

  switch (state)
  {
  case WATCH:
    if (digitalRead(signalInput) != relaisState)
    {
      previousMillis = currentMillis;
      state = WAIT;
    }
    break;

  case WAIT:
    if (digitalRead(signalInput) == relaisState)
    {
      state = WATCH;
    } else
    if (currentMillis - previousMillis >= timeout)
    {
      relaisState = !relaisState;
    }
    break;

  default:
    break;
  }

  digitalWrite(signalOutput, relaisState);
}