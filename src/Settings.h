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
  //change eeprom config version ONLY when new parameter is added and need reset the parameter
  unsigned int configVersion = 10;
public:
  struct Data{         // do not re-sort this struct
    unsigned int coVers;      //config version, if changed, previus config will erased
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
    data = {}; // clear bevor load data
    memset(&data, 0, sizeof data);
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
    //memset(&data, 0, sizeof data);
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
      strcpy(data.mqttServer,"-1");
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
      strcpy(data.mqttTopic , "BMS01");
    }
    if(data.mqttPort <= 0 || data.mqttPort >= 65530){
      data.mqttPort = 0;
    }
    if(data.mqttRefresh <= 1 || data.mqttRefresh >= 65530){
      data.mqttRefresh = 1;
    }
    if(data.mqttJson != true || data.mqttJson != false){
      data.mqttJson = false;
    }
  }
  void coVersCheck()
  {
      if(data.coVers != configVersion)
      {
        data.coVers = configVersion;
        strcpy(data.deviceName, "DALY-BMS-to-MQTT");
        strcpy(data.mqttServer,"-1");
        strcpy(data.mqttUser, "");
        strcpy(data.mqttPassword, "");
        strcpy(data.mqttTopic , "BMS01");
        data.mqttPort = 0;
        data.mqttRefresh = 1;
        data.mqttJson = false;
        save();
        load();
      }
  }
};
