//Settings: Stores persistant settings, loads and saves to EEPROM

#include <arduino.h>
#include <EEPROM.h>

//#define SERIALDEBUG //enable Serial Output for debug
#define MQTTDEBUG //enable debug / RAW Messages from iverter to MQTT

class Settings
{
public:
  bool _valid = false;
  //MQTT Settings
  //String _deviceType = "PIP"; //PIP | PCM | MPI
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
    #ifdef SERIALDEBUG
    Serial1.print("Writing ");
    Serial1.print(maxLen);
    Serial1.print(" ");
    Serial1.print(offset);
    Serial1.print(" ");
    Serial1.println(s);
    #endif

    for (i = 0; i < maxLen; ++i)
    {
      EEPROM.write(offset + i, s[i]);
      #ifdef SERIALDEBUG
      Serial1.print(" ");
      Serial1.print(offset + i);
      Serial1.print("=");
      Serial1.print(s[i]);
      Serial1.print(",");
      #endif
    }
    //null terminate the string
    EEPROM.write(offset + i, 0);
    #ifdef SERIALDEBUG
    Serial1.print(offset + i);
    #endif
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
      //readString(_deviceType, 0x20, 0x60);
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
    //writeString(_deviceType, 0x20, 0x60);
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
  _mqttRefresh = 0;
  save();
  delay(500);
  }

  Settings()
  {
    load();
  }
};
