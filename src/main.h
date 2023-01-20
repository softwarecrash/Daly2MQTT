//#include "Arduino.h"
/**
 * @brief main function for the relais
 */
bool relaisHandler();

/**
 * @brief main function for the rwakeup function
 */
bool wakeupHandler();

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