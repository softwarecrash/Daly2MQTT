/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1024

class Settings
{
public:
  struct {         // do not re-sort this struct
    char deviceName[40];      // device name
    char mqttServer[40];      // mqtt Server adress
    char mqttUser[40];        // mqtt Username
    char mqttPassword[40];    // mqtt Password
    char mqttTopic[40];       // mqtt publish topic
    unsigned int mqttPort;    // mqtt port
    unsigned int mqttRefresh; // mqtt refresh time
    bool mqttJson;            // switch between classic mqtt and json
  }data;

  void load()
  {
    data = {}; //clear bevor load data
    memset(&data, 0, sizeof data);
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, data);
    EEPROM.end();
  }

  void save()
  {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, data);
    EEPROM.commit();
    EEPROM.end();
  }

  void reset(){
  data = {};
  memset(&data, 0, sizeof data);
  save();
  }
};

