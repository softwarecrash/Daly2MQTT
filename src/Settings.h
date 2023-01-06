/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

#include <Arduino.h>
#include <EEPROM.h>

#define NEW_EEPROM

#define EEPROM_SIZE 1024



#ifndef NEW_EEPROM
class Settings
{
public:
  bool _valid = false;
  //MQTT Settings
  bool _mqttJson = false;
  String _deviceName = "";    //name of the device
  String _mqttServer = "";    //host or ip from the mqtt server
  String _mqttUser = "";      //mqtt username to login
  String _mqttPassword = "";  //mqtt passwort
  String _mqttTopic = "";     //MQTT Topic
  short _mqttPort = 0;        //mqtt server port
  short _mqttRefresh = 0;     //mqtt Send Interval in Seconds

  short readShort(int offset)
  {
    byte b1 = EEPROM.read(offset + 0);
    byte b2 = EEPROM.read(offset + 1);
    return ((short)b1 << 8) | b2;
  }

  void writeShort(short value, int offset)
  {
    byte b1 = (byte)((value >> 8) & 0xFF);
    byte b2 = (byte)((value >> 0) & 0xFF);

    EEPROM.write(offset + 0, b1);
    EEPROM.write(offset + 1, b2);
  }

  void readString(String &s, int maxLen, int offset)
  {
    int i;
    s = "";
    for (i = 0; i < maxLen; ++i)
    {
      char c = EEPROM.read(offset + i);
      if (c == 0)
        break;
      s += c;
    }
  }

  void writeString(String &s, int maxLen, int offset)
  {
    int i;
    //leave space for null termination
    maxLen--;
    if ((int)s.length() < maxLen - 1)
      maxLen = s.length();

    for (i = 0; i < maxLen; ++i)
    {
      EEPROM.write(offset + i, s[i]);
    }
    //null terminate the string
    EEPROM.write(offset + i, 0);
  }

  void load()
  {
    EEPROM.begin(512);

    _valid = true;
    _valid &= EEPROM.read(0) == 0xDB;
    _valid &= EEPROM.read(1) == 0xEE;
    _valid &= EEPROM.read(2) == 0xAE;
    _valid &= EEPROM.read(3) == 0xDF;

    if (_valid)
    {
      _mqttRefresh = readShort(0x20);
      readString(_mqttTopic, 0x20, 0x40);
      if(readShort(0x60) == 10) _mqttJson = true;
      if(readShort(0x60) == 00) _mqttJson = false;
      readString(_deviceName, 0x20, 0x80);
      readString(_mqttServer, 0x20, 0xA0);
      readString(_mqttPassword, 0x20, 0xC0);
      readString(_mqttUser, 0x20, 0xE0);
      _mqttPort = readShort(0x100);
    }

    EEPROM.end();
  }

  void save()
  {
    EEPROM.begin(512);

    EEPROM.write(0, 0xDB);
    EEPROM.write(1, 0xEE);
    EEPROM.write(2, 0xAE);
    EEPROM.write(3, 0xDF);

    writeShort(_mqttRefresh, 0x20);
    writeString(_mqttTopic, 0x20, 0x40);
    if(_mqttJson == true) writeShort((10), 0x60);
    if(_mqttJson == false) writeShort((00), 0x60);
    writeString(_deviceName, 0x20, 0x80);
    writeString(_mqttServer, 0x20, 0xA0);
    writeString(_mqttPassword, 0x20, 0xC0);
    writeString(_mqttUser, 0x20, 0xE0);
    writeShort(_mqttPort, 0x100);

    EEPROM.commit();

    _valid = true;

    EEPROM.end();
  }

  void reset(){
  _deviceName = "";
  _mqttServer = "";
  _mqttUser = "";
  _mqttPassword = "";
  _mqttTopic = "";
  _mqttPort = 0;
  _mqttRefresh = 10;
  _mqttJson = false;
  save();
  delay(500);
  }

  Settings()
  {
    load();
  }
};
#endif

#ifdef NEW_EEPROM
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
  //eeprom_data data;

  //bool _mqttJson = false;
  //String _deviceName = "";    //name of the device
  //String _mqttServer = "";    //host or ip from the mqtt server
  //String _mqttUser = "";      //mqtt username to login
  //String _mqttPassword = "";  //mqtt passwort
  //String _mqttTopic = "";     //MQTT Topic
  //short _mqttPort = 0;        //mqtt server port
  //short _mqttRefresh = 0;     //mqtt Send Interval in Seconds


  void load()
  {
    data = {}; //clear bevor load data
    memset(&data, 0, sizeof data);
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, data);
    EEPROM.end();

    //_deviceName = data.deviceName;
    //_mqttServer = data.mqttServer;
    //_mqttUser = data.mqttUser;
    //_mqttPassword = data.mqttPassword;
    //_mqttTopic = data.mqttTopic;


    //_mqttPort = data.mqttPort;
    //_mqttRefresh = data.mqttRefresh;
    //_mqttJson = data.mqttJson;
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

 // Settings()
  //{
 //   load();
 // }
};
#endif
