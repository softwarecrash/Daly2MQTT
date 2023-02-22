/***
 1 mal blinken status, alles ok
 2 mal blinken bms fehler
 3 mal blinken mqtt fehler
 4 mal blinken wlan fehler

*/

void notificationLED(){



  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }



}