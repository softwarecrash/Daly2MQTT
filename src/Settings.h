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
  // change eeprom config version ONLY when new parameter is added and need reset the parameter
  unsigned int configVersion = 10;

public:
  struct Data
  {                             // do not re-sort this struct
    unsigned int coVers;        // config version, if changed, previus config will erased
    char deviceName[40];        // device name
    char mqttServer[40];        // mqtt Server adress
    char mqttUser[40];          // mqtt Username
    char mqttPassword[40];      // mqtt Password
    char mqttTopic[40];         // mqtt publish topic
    unsigned int mqttPort;      // mqtt port
    unsigned int mqttRefresh;   // mqtt refresh time
    bool mqttJson;              // switch between classic mqtt and json
    bool wakeupEnable = false;  // use wakeup output?
    bool wakeupInvert = false;  // invert wakeup output?
    bool relaisEnable = false;  // enable relais output?
    bool relaisInvert = false;  // invert relais output?
    byte relaisFunction = 0;    // function mode - 0 = Lowest Cell Voltage, 1 = Highest Cell Voltage, 2 = Pack Cell Voltage, 3 = Temperature
    byte relaisComparsion = 0;  // comparsion mode - 0 = Higher or equal than, 1 = Lower or equal than
    float relaissetvalue = 0.0; // value to compare to !!RENAME TO SOMETHING BETTER!!
    float relaisHysteresis = 0.0; // value to compare to
  } data;

  void load()
  {
    data = {}; // clear bevor load data
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, data);
    EEPROM.end();
    coVersCheck();
    sanitycheck();
  }

  void save()
  {
    sanitycheck();
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, data);
    EEPROM.commit();
    EEPROM.end();
  }

  void reset()
  {
    data = {};
    save();
  }

private:
  // check the variables from eeprom

  void sanitycheck()
  {
    if (strlen(data.deviceName) == 0 || strlen(data.deviceName) >= 40)
    {
      strcpy(data.deviceName, "DALY-BMS-to-MQTT");
    }
    if (strlen(data.mqttServer) == 0 || strlen(data.mqttServer) >= 40)
    {
      strcpy(data.mqttServer, "-1");
    }
    if (strlen(data.mqttUser) == 0 || strlen(data.mqttUser) >= 40)
    {
      strcpy(data.mqttUser, "");
    }
    if (strlen(data.mqttPassword) == 0 || strlen(data.mqttPassword) >= 40)
    {
      strcpy(data.mqttPassword, "");
    }
    if (strlen(data.mqttTopic) == 0 || strlen(data.mqttTopic) >= 40)
    {
      strcpy(data.mqttTopic, "BMS01");
    }
    if (data.mqttPort <= 0 || data.mqttPort >= 65530)
    {
      data.mqttPort = 0;
    }
    if (data.mqttRefresh <= 1 || data.mqttRefresh >= 65530)
    {
      data.mqttRefresh = 1;
    }
    if (data.mqttJson && !data.mqttJson)
    {
      data.mqttJson = false;
    }
    if (data.wakeupEnable && !data.wakeupEnable)
    {
      data.wakeupEnable = false;
    }
    if (data.wakeupInvert && !data.wakeupInvert)
    {
      data.wakeupInvert = false;
    }
    if (data.relaisEnable && !data.relaisEnable)
    {
      data.relaisEnable = false;
    }
    if (data.relaisInvert && !data.relaisInvert)
    {
      data.relaisInvert = false;
    }
    if (data.relaisFunction < 0 || data.relaisFunction > 4)
    {
      data.relaisFunction = 0;
    }
    if (data.relaisComparsion < 0 || data.relaisComparsion > 1)
    {
      data.relaisComparsion = 0;
    }
    if (data.relaissetvalue < -100 || data.relaissetvalue > 100)
    {
      data.relaissetvalue = 0;
    }
    if (data.relaisHysteresis < -100 || data.relaisHysteresis > 100)
    {
      data.relaisHysteresis = 0;
    }
  }
  void coVersCheck()
  {
    if (data.coVers != configVersion)
    {
      data.coVers = configVersion;
      strcpy(data.deviceName, "DALY-BMS-to-MQTT");
      strcpy(data.mqttServer, "-1");
      strcpy(data.mqttUser, "");
      strcpy(data.mqttPassword, "");
      strcpy(data.mqttTopic, "BMS01");
      data.mqttPort = 0;
      data.mqttRefresh = 300;
      data.mqttJson = false;
      data.wakeupEnable = false;
      data.wakeupInvert = false;
      data.relaisEnable = false;
      data.relaisInvert = false;
      data.relaisFunction = 0;
      data.relaisComparsion = 0;
      data.relaissetvalue = 0.0;
      data.relaisHysteresis = 0.0;
      save();
      load();
    }
  }
};
