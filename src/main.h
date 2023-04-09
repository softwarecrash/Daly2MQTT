#ifdef isDEBUG
#define DALY_BMS_DEBUG Serial // Uncomment the below #define to enable debugging print statements.
#endif

#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_LONG_LONG 0
#define MYPORT_TX 12
#define MYPORT_RX 13

#define WAKEUP_PIN 15 // GPIO pin for the wakeup transistor
#define RELAIS_PIN 14  // GPIO pin for relais

#define LED_PIN 02 //D4 with the LED on Wemos D1 Mini

#define RELAISINTERVAL 1000   // interval for relaisHandler()
//#define WAKEUP_INTERVAL 10000 // interval for wakeupHandler()
#define WAKEUP_DURATION 250   // duration how long the pin is switched
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
#define ESP01 "display: none;"
#endif 
#endif
/*
#ifdef WAKEUP_PIN
#undef WAKEUP_PIN
#define WAKEUP_PIN 0 // GPIO pin for the wakeup transistor
#endif
#ifdef RELAIS_PIN
#undef RELAIS_PIN
#define RELAIS_PIN 2  // GPIO pin for relais
#endif */
#endif

#define JSON_BUFFER 2048
#define MQTT_BUFFER 512

// DON'T edit version here, place version number in platformio.ini (custom_prog_version) !!!
#define SOFTWARE_VERSION SWVERSION
#ifdef DALY_BMS_DEBUG
#undef SOFTWARE_VERSION
#define SOFTWARE_VERSION SWVERSION " " HWBOARD " " __DATE__ " " __TIME__
#endif

#define FlashSize ESP.getFlashChipSize()

#ifdef DALY_BMS_DEBUG
#define DEBUG_BEGIN(...) DALY_BMS_DEBUG.begin(__VA_ARGS__)
#define DEBUG_PRINT(...) DALY_BMS_DEBUG.print(__VA_ARGS__)
#define DEBUG_PRINTF(...) DALY_BMS_DEBUG.printf(__VA_ARGS__)
#define DEBUG_WRITE(...) DALY_BMS_DEBUG.write(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DALY_BMS_DEBUG.println(__VA_ARGS__)
#else
#undef DEBUG_BEGIN
#undef DEBUG_PRINT
#undef DEBUG_PRINTF
#undef DEBUG_WRITE
#undef DEBUG_PRINTLN
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTF(...)
#define DEBUG_WRITE(...)
#define DEBUG_PRINTLN(...)
#endif

/**
 * @brief function for uart callback to prozess avaible data
 * 
 */
void prozessUartData();
/**
 * @brief main function for the relais
 */
bool relaisHandler();

/**
 * @brief main function for the rwakeup function
 */
bool wakeupHandler();

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