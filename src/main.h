/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
#ifdef isDEBUG
#define DALY_BMS_DEBUG Serial // Uncomment the below #define to enable debugging print statements.
#include <WebSerialLite.h>
#define DEBUG_SHOW_HTML
#endif

#ifndef isDEBUG
#define DEBUG_SHOW_HTML "display:none;"
#endif

#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_LONG_LONG 0
#define MYPORT_TX 12
#define MYPORT_RX 13

#define WAKEUP_PIN 15 // GPIO pin for the wakeup transistor
#define RELAIS_PIN 14  // GPIO pin for relais

#define LED_PIN 02 //D4 with the LED on Wemos D1 Mini

#define RELAISINTERVAL 1000   // interval for relaisHandler()
#define WAKEUP_DURATION 250   // duration for wakeupHandler()
#define ESP01

#ifdef ARDUINO_ESP8266_ESP01
#ifdef MYPORT_TX
#undef MYPORT_TX
#define MYPORT_TX 0
#endif
#ifdef MYPORT_RX
#undef MYPORT_RX
#define MYPORT_RX 2
#ifdef ESP01
#undef ESP01
#define ESP01 "display:none;"
#endif 
#endif
#endif

#define JSON_BUFFER 2048
#define DEBUG_BAUD 115200

// DON'T edit version here, place version number in platformio.ini (custom_prog_version) !!!
#define SOFTWARE_VERSION SWVERSION
#ifdef DALY_BMS_DEBUG
#undef SOFTWARE_VERSION
#define SOFTWARE_VERSION SWVERSION " " HWBOARD " " __DATE__ " " __TIME__
#endif

#define FlashSize ESP.getFreeSketchSpace()

#ifdef DALY_BMS_DEBUG
#define DEBUG_BEGIN(...) DALY_BMS_DEBUG.begin(__VA_ARGS__)
#define DEBUG_END(...) DALY_BMS_DEBUG.end(__VA_ARGS__)
#define DEBUG_PRINT(...) DALY_BMS_DEBUG.print(__VA_ARGS__)
#define DEBUG_PRINTF(...) DALY_BMS_DEBUG.printf(__VA_ARGS__)
#define DEBUG_WRITE(...) DALY_BMS_DEBUG.write(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DALY_BMS_DEBUG.println(__VA_ARGS__)
#define DEBUG_WEB(...) WebSerial.print(__VA_ARGS__)
#define DEBUG_WEBLN(...) WebSerial.println(__VA_ARGS__)
#else
#undef DEBUG_BEGIN
#undef DEBUG_PRINT
#undef DEBUG_PRINTF
#undef DEBUG_WRITE
#undef DEBUG_PRINTLN
#undef DEBUG_WEB
#undef DEBUG_WEBLN
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTF(...)
#define DEBUG_WRITE(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_WEB(...)
#define DEBUG_WEBLN(...)
#endif

/**
 * @brief function for uart callback to prozess avaible data
 * 
 */
void prozessData();
/**
 * @brief main function for the relais
 */
bool relaisHandler();

/**
 * @brief counter function for reset reason
 */
bool resetCounter(bool count);

/**
 * @brief main function for the rwakeup function
 */
bool wakeupHandler(bool wakeIt);

/**
 * @brief get the basic device data 
 * 
 */
void getJsonDevice();

/**
 * @brief read the data from bms and put it in the json
 */
void getJsonData();

/**
 * @brief prozess the mqtt callbacks
 * @note payload was bevor byte, but byte is typedef of unsigned char
 */
void mqttcallback(char *topic, unsigned char *payload, unsigned int length);

/**
 * @brief in case of wrong or no data, clear the json
 */
void clearJsonData();

/**
 * @brief mqtt connector function
 */
bool connectMQTT();

/**
 * @brief function that send all the data to the mqtt client
 */
bool sendtoMQTT();

/**
 * @brief function that send all the data to the mqtt client
 */
void notificationLED();

/**
 * @brief function fires up the discovery for HA
 */
bool sendHaDiscovery();
static const char *const haPackDescriptor[][4]{
{"Name", "mdi:tournament", "", ""},
{"Device_IP", "mdi:ip-network", "", ""},
{"Wifi_RSSI", "mdi:wifi-arrow-up-down", "dBa", "signal_strength"},
{"RelaisOutput_Active", "mdi:wifi-arrow-up-down", "", ""},
{"RelaisOutput_Manual", "mdi:wifi-arrow-up-down", "", ""},
{"sw_version", "", "", ""},

{"Pack_Voltage", "mdi:car-battery", "V", "voltage"},
{"Pack_Current", "mdi:current-dc", "A", "current"},
{"Pack_Power", "mdi:home-battery", "W", "power"},
{"Pack_SOC", "mdi:battery-charging-high", "%", "battery"},
{"Pack_Remaining_mAh", "mdi:battery", "mAh", ""},
{"Pack_Cycles", "mdi:counter", "", "counter"},
{"Pack_BMS_Temperature", "mdi:battery", "°C", "temperature"},
{"Pack_Cell_High", "mdi:battery", "", ""},
{"Pack_Cell_High_Voltage", "mdi:battery-high", "V", "voltage"},
{"Pack_Cell_Low", "mdi:battery-outline", "", ""},
{"Pack_Cell_Low_Voltage", "mdi:battery-outline", "V", "voltage"},
{"Pack_Cell_Difference", "mdi:scale-balance:", "mV", "voltage"},
{"Pack_DischargeFET", "mdi:battery-outline", "", ""},
{"Pack_ChargeFET", "mdi:battery-high", "", ""},
{"Pack_Status", "", "", ""},
{"Pack_Cells", "mdi:counter", "", "counter"},
{"Pack_Heartbeat", "mdi:counter", "", "counter"},
{"Pack_Balance_Active", "mdi:scale-balance", "", ""},
{"Pack_Failure", "", "", ""},
};
static const char *const haCellTempDescriptor[][4]{
// insert number from 
{"Cell_Temp_", "mdi:thermometer-lines", "°C", "temperature"}
};
static const char *const haControlDescriptor[][4]{
{"Pack_ChargeFET", "", "", ""},
{"Pack_DischargeFET", "", "", ""},
//{"Pack_SOC", "", "", ""},
{"Pack_Relais", "", "", ""}
};