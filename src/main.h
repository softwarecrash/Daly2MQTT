//#include "Arduino.h"
/**
 * @brief main function for the relais
 * @details some aditional text here
 */
bool relaisHandler();

/**
 * @brief main function for the rwakeup function
 * @details some aditional text here
 */
bool wakeupHandler();

/**
 * @brief read the data from bms and put it in the json
 * @details some aditional text here
 */
void getJsonData();

/**
 * @brief prozess the mqtt callbacks
 * @details some aditional text here
 * @note payload was bevor byte, but byte is typedef of unsigned char
 */
void mqttcallback(char *topic, unsigned char *payload, unsigned int length);

/**
 * @brief in case of wrong or no data, clear the json
 * @details some aditional text here
 */
void clearJsonData();

/**
 * @brief mqtt connector function
 * @details some aditional text here
 */
bool connectMQTT();

/**
 * @brief function that send all the data to the mqtt client
 * @details some aditional text here
 */
bool sendtoMQTT();