/*#ifdef DEBUG_SERIAL
//make it better like
//https://stackoverflow.com/questions/28931195/way-to-toggle-debugging-code-on-and-off
#define DEBBUG_BEGIN(...) DEBUG_SERIAL.begin(__VA_ARGS__)
#define DEBUG_PRINT(...) DEBUG_SERIAL.print(__VA_ARGS__)
#define DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#define DEBUG_WRITE(...) DEBUG_SERIAL.write(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DEBUG_SERIAL.println(__VA_ARGS__)
#else
#undef DEBBUG_BEGIN
#undef DEBUG_PRINT
#undef DEBUG_PRINTF
#undef DEBUG_WRITE
#undef DEBUG_PRINTLN
#define DEBBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTF(...)
#define DEBUG_WRITE(...)
#define DEBUG_PRINTLN(...)
#endif*/


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