#define DALY_BMS_DEBUG Serial // Uncomment the below #define to enable debugging print statements.

#ifdef DALY_BMS_DEBUG
//make it better like
//https://stackoverflow.com/questions/28931195/way-to-toggle-debugging-code-on-and-off
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