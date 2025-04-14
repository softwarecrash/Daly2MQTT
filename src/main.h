/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/

#define DEBUG_SERIAL Serial // Uncomment the below #define to enable debugging print statements.
//include <WebSerialLite.h>
#include <MycilaWebSerial.h>

#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_LONG_LONG 0
#define MYPORT_TX 12
#define MYPORT_RX 13

#define WAKEUP_PIN 15 // GPIO pin for the wakeup transistor
#define RELAIS_PIN 14 // GPIO pin for relais

#define LED_PIN 02 // D4 with the LED on Wemos D1 Mini

#define TEMPSENS_PIN 04 // DS18B20 Pin
#define TIME_INTERVAL 1500 // Time interval among sensor readings [milliseconds]
//#define DEBUG_DS18B20 // uncomment for Debug

#define RELAISINTERVAL 1000 // interval for relaisHandler()
#define WAKEUP_DURATION 250 // duration for wakeupHandler()
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

#define JSON_BUFFER 3027//2304
#define DBG_BAUD 115200

// DON'T edit version here, place version number in platformio.ini (custom_prog_version) !!!
#define SOFTWARE_VERSION SWVERSION

#define FlashSize ESP.getFreeSketchSpace()


#define DBG_BEGIN(...) DEBUG_SERIAL.begin(__VA_ARGS__)
#define DBG_PRINTLN(...) DEBUG_SERIAL.println(__VA_ARGS__)
#define DBG_WEBLN(...) webSerial.println(__VA_ARGS__)


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

/**
 * @brief function for ext. TempSensors
 */
void handleTemperatureChange(int deviceIndex, int32_t temperatureRAW);

/**
 * @brief this function act like s/n/printf() and give the output to the configured serial and webserial
 *
 */
void writeLog(const char* format, ...);

static const char *const haPackDescriptor[][4]{
    {"Device_IP", "ip-network", "", ""},
    {"Wifi_RSSI", "wifi-arrow-up-down", "dB", "signal_strength"},
    //{"Pack_Relais", "electric-switch", "", ""},
    {"Pack_Relais_Manual", "electric-switch", "", ""},
    {"Pack_Voltage", "car-battery", "V", "voltage"},
    {"Pack_Current", "current-dc", "A", "current"},
    {"Pack_Power", "home-battery", "W", "power"},
    {"Pack_SOC", "battery-charging-high", "%", "battery"},
    //{"Pack_Remaining_Ah", "battery", "Ah", "energy_storage"},
    //{"Pack_Remaining_Ah", "battery", "Ah", ""}, /7remove? HA canot regognize Ah
    {"Pack_Remaining_kWh", "battery", "kWh", "energy_storage"}, // new
    {"Pack_Cycles", "counter", "", ""},
    {"Pack_BMS_Temperature", "battery", "°C", "temperature"},
    {"Pack_Cell_High", "battery", "", ""},
    {"Pack_Cell_High_Voltage", "battery-high", "V", "voltage"},
    {"Pack_Cell_Low", "battery-outline", "", ""},
    {"Pack_Cell_Low_Voltage", "battery-outline", "V", "voltage"},
    {"Pack_Cell_Difference", "scale-balance:", "mV", "voltage"},
    //{"Pack_DischargeFET", "battery-outline", "", ""},
    //{"Pack_ChargeFET", "battery-high", "", ""},
    {"Pack_Status", "state-machine", "", ""},
    {"Pack_Cells", "counter", "", ""},
    {"Pack_Heartbeat", "counter", "", ""},
    {"Pack_Balance_Active", "scale-balance", "", ""},
    {"Pack_Failure", "alert-circle-outline", "", ""},
};
static const char *const haControlDescriptor[][4]{
    {"Pack_ChargeFET", "toggle-switch-off", "", ""},
    {"Pack_DischargeFET", "toggle-switch-off", "", ""},
    //{"Pack_SOC", "toggle-switch-off", "", ""},
    {"Pack_Relais", "toggle-switch-off", "", ""}};