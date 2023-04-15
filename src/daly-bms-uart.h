/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/
#include "SoftwareSerial.h"
#ifndef DALY_BMS_UART_H
#define DALY_BMS_UART_H

#define XFER_BUFFER_LENGTH 13
#define MIN_NUMBER_CELLS 1
#define MAX_NUMBER_CELLS 48
#define MIN_NUMBER_TEMP_SENSORS 1
#define MAX_NUMBER_TEMP_SENSORS 16

#define START_BYTE 0xA5; // Start byte
#define HOST_ADRESS 0x40; // Host address
#define FRAME_LENGTH 0x08; // Length

//time in ms for delay the bms requests, to fast brings connection error
#define DELAYTINME 250

// DON'T edit DEBUG here, edit build_type in platformio.ini !!!
#ifdef isDEBUG
#define DEBUG_SERIAL Serial
#endif

#ifdef DEBUG_SERIAL
//make it better like
//https://stackoverflow.com/questions/28931195/way-to-toggle-debugging-code-on-and-off
#define BMS_DEBUG_BEGIN(...) DEBUG_SERIAL.begin(__VA_ARGS__)
#define BMS_DEBUG_PRINT(...) DEBUG_SERIAL.print(__VA_ARGS__)
#define BMS_DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#define BMS_DEBUG_WRITE(...) DEBUG_SERIAL.write(__VA_ARGS__)
#define BMS_DEBUG_PRINTLN(...) DEBUG_SERIAL.println(__VA_ARGS__)
#else
#undef BMS_DEBUG_BEGIN
#undef BMS_DEBUG_PRINT
#undef BMS_DEBUG_PRINTF
#undef BMS_DEBUG_WRITE
#undef BMS_DEBUG_PRINTLN
#define BMS_DEBBUG_BEGIN(...)
#define BMS_DEBUG_PRINT(...)
#define BMS_DEBUG_PRINTF(...)
#define BMS_DEBUG_WRITE(...)
#define BMS_DEBUG_PRINTLN(...)
#endif

class Daly_BMS_UART
{
public:
    unsigned int previousTime = 0;
    byte requestCounter = 0;
    int soft_tx;
    int soft_rx;

    enum COMMAND
    {
        VOUT_IOUT_SOC = 0x90,
        MIN_MAX_CELL_VOLTAGE = 0x91,
        MIN_MAX_TEMPERATURE = 0x92,
        DISCHARGE_CHARGE_MOS_STATUS = 0x93,
        STATUS_INFO = 0x94,
        CELL_VOLTAGES = 0x95,
        CELL_TEMPERATURE = 0x96,
        CELL_BALANCE_STATE = 0x97,
        FAILURE_CODES = 0x98,
        DISCHRG_FET = 0xD9,
        CHRG_FET = 0xDA,
        BMS_RESET = 0x00,
        READ_SOC = 0x61, //read the time and soc
        SET_SOC = 0x21, //set the time and soc
        //END = 0xD8,
        //after request the pc soft hangs a 0xD8 as last request, its empty, dont know what it means?
    };

    /**
     * @brief get struct holds all the data collected from the BMS and is populated using the update() API
     */
    struct
    {
        // data from 0x90
        float packVoltage; // pressure (0.1 V)
        float packCurrent; // acquisition (0.1 V)
        float packSOC;     // State Of Charge

        // data from 0x91
        float maxCellmV; // maximum monomer voltage (mV)
        int maxCellVNum; // Maximum Unit Voltage cell No.
        float minCellmV; // minimum monomer voltage (mV)
        int minCellVNum; // Minimum Unit Voltage cell No.
        int cellDiff;  // difference betwen cells

        // data from 0x92
        int tempAverage; // Avergae Temperature

        // data from 0x93
        const char *chargeDischargeStatus; // charge/discharge status (0 stationary ,1 charge ,2 discharge)
        bool chargeFetState;          // charging MOS tube status
        bool disChargeFetState;       // discharge MOS tube state
        int bmsHeartBeat;             // BMS life(0~255 cycles)
        int resCapacitymAh;           // residual capacity mAH

        // data from 0x94
        unsigned int numberOfCells;    // amount of cells
        unsigned int numOfTempSensors; // amount of temp sensors
        bool chargeState;     // charger status 0=disconnected 1=connected
        bool loadState;       // Load Status 0=disconnected 1=connected
        bool dIO[8];          // No information about this
        int bmsCycles;        // charge / discharge cycles

        // data from 0x95
        float cellVmV[48]; // Store Cell Voltages in mV

        // data from 0x96
        int cellTemperature[16]; // array of cell Temperature sensors

        // data from 0x97
        bool cellBalanceState[48]; // bool array of cell balance states
        bool cellBalanceActive;    // bool is cell balance active

        // get a state of the connection
        int connectionState;

    } get;

    /**
     * @brief alarm struct holds booleans corresponding to all the possible alarms
     * (aka errors/warnings) the BMS can report
     */

    struct
    {
        // data from 0x98
        /* 0x00 */
        bool levelOneCellVoltageTooHigh;
        bool levelTwoCellVoltageTooHigh;
        bool levelOneCellVoltageTooLow;
        bool levelTwoCellVoltageTooLow;
        bool levelOnePackVoltageTooHigh;
        bool levelTwoPackVoltageTooHigh;
        bool levelOnePackVoltageTooLow;
        bool levelTwoPackVoltageTooLow;

        /* 0x01 */
        bool levelOneChargeTempTooHigh;
        bool levelTwoChargeTempTooHigh;
        bool levelOneChargeTempTooLow;
        bool levelTwoChargeTempTooLow;
        bool levelOneDischargeTempTooHigh;
        bool levelTwoDischargeTempTooHigh;
        bool levelOneDischargeTempTooLow;
        bool levelTwoDischargeTempTooLow;

        /* 0x02 */
        bool levelOneChargeCurrentTooHigh;
        bool levelTwoChargeCurrentTooHigh;
        bool levelOneDischargeCurrentTooHigh;
        bool levelTwoDischargeCurrentTooHigh;
        bool levelOneStateOfChargeTooHigh;
        bool levelTwoStateOfChargeTooHigh;
        bool levelOneStateOfChargeTooLow;
        bool levelTwoStateOfChargeTooLow;

        /* 0x03 */
        bool levelOneCellVoltageDifferenceTooHigh;
        bool levelTwoCellVoltageDifferenceTooHigh;
        bool levelOneTempSensorDifferenceTooHigh;
        bool levelTwoTempSensorDifferenceTooHigh;

        /* 0x04 */
        bool chargeFETTemperatureTooHigh;
        bool dischargeFETTemperatureTooHigh;
        bool failureOfChargeFETTemperatureSensor;
        bool failureOfDischargeFETTemperatureSensor;
        bool failureOfChargeFETAdhesion;
        bool failureOfDischargeFETAdhesion;
        bool failureOfChargeFETTBreaker;
        bool failureOfDischargeFETBreaker;

        /* 0x05 */
        bool failureOfAFEAcquisitionModule;
        bool failureOfVoltageSensorModule;
        bool failureOfTemperatureSensorModule;
        bool failureOfEEPROMStorageModule;
        bool failureOfRealtimeClockModule;
        bool failureOfPrechargeModule;
        bool failureOfVehicleCommunicationModule;
        bool failureOfIntranetCommunicationModule;

        /* 0x06 */
        bool failureOfCurrentSensorModule;
        bool failureOfMainVoltageSensorModule;
        bool failureOfShortCircuitProtection;
        bool failureOfLowVoltageNoCharging;
    } alarm;

    /**
     * @brief Construct a new Daly_BMS_UART object
     *
     * @param serialIntf UART interface BMS is connected to
     */
    Daly_BMS_UART(int rx, int tx);

    /**
     * @brief Initializes this driver
     * @details Configures the serial peripheral and pre-loads the transmit buffer with command-independent bytes
     */
    bool Init();

    /**
     * @brief Updating the Data from the BMS
     */
    bool update();

    /**
     * @brief Gets Voltage, Current, and SOC measurements from the BMS
     * @return True on successful aquisition, false otherwise
     */
    bool getPackMeasurements();

    /**
     * @brief Gets the pack temperature from the min and max of all the available temperature sensors
     * @details Populates tempMax, tempMax, and tempAverage in the "get" struct
     * @return True on successful aquisition, false otherwise
     */
    bool getPackTemp();

    /**
     * @brief Returns the highest and lowest individual cell voltage, and which cell is highest/lowest
     * @details Voltages are returned as floats with milliVolt precision (3 decimal places)
     * @return True on successful aquisition, false otherwise
     */
    bool getMinMaxCellVoltage();

    /**
     * @brief Get the general Status Info
     *
     */
    bool getStatusInfo();

    /**
     * @brief Get Cell Voltages
     *
     */
    bool getCellVoltages();

    /**
     * @brief   Each temperature accounts for 1 byte, according to the
                actual number of temperature send, the maximum 21
                byte, send in 3 frames
                Byte0:frame number, starting at 0
                Byte1~byte7:cell temperature(40 Offset ,℃)
     *
     */
    bool getCellTemperature();

    /**
     * @brief   0： Closed 1： Open
                Bit0: Cell 1 balance state
                ...
                Bit47:Cell 48 balance state
                Bit48~Bit63：reserved
     *
     */
    bool getCellBalanceState();

    /**
     * @brief Get the Failure Codes
     *
     */
    bool getFailureCodes();

    /**
     * @brief
     * set the Discharging MOS State
     */
    bool setDischargeMOS(bool sw);

    /**
     * @brief set the Charging MOS State
     *
     */
    bool setChargeMOS(bool sw);

    /**
     * @brief set the SOC
     *
     */
    bool setSOC(float sw);

    /**
     * @brief Read the charge and discharge MOS States
     *
     */
    bool getDischargeChargeMosStatus();

    /**
     * @brief Reseting The BMS
     * @details Reseting the BMS and let it restart
     */
    bool setBmsReset();

    /**
     * @brief return the state of connection to the BMS
     * @details returns the following value for different connection state
     * -3 - could not open serial port
     * -2 - no data recived or wrong crc, check connection
     * -1 - working and collecting data, please wait
     *  0 - All data recived with correct crc, idleing
     */
    int getState();

    /**
     * @brief callback function
     * 
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback;

private:

    /**
     * @brief send the command id, and return true if data complete read or false by crc error
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    bool requestData(COMMAND cmdID, unsigned int frameAmount);

    /**
     * @brief Sends a complete packet with the specified command
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    void sendCommand(COMMAND cmdID);

    /**
     * @brief Send the command ID to the BMS
     * @details
     * @return True on success, false on failure
     */
    bool receiveBytes(void);

    /**
     * @brief Send the command ID to the BMS
     * @details
     * @return True on success, false on failure
     */
    bool receiveFrames(unsigned int frameAmount);

    /**
     * @brief Validates the checksum in the RX Buffer
     * @return true if checksum matches, false otherwise
     */
    bool validateChecksum();

    /**
     * @brief Prints out the contense of the RX buffer
     * @details Useful for debugging
     */
    void barfRXBuffer();

    /**
     * @brief Clear all data from the Get struct
     * @details when wrong or missing data comes in it need sto be cleared
     */
    void clearGet();

    /**
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    SoftwareSerial *my_serialIntf;

    /**
     * @brief Buffer used to transmit data to the BMS
     * @details Populated primarily in the "Init()" function, see the readme for more info
     */
    uint8_t my_txBuffer[XFER_BUFFER_LENGTH];

    /**
     * @brief Buffer filled with data from the BMS
     */
    uint8_t my_rxBuffer[XFER_BUFFER_LENGTH];


uint8_t my_rxFrameBuffer[XFER_BUFFER_LENGTH*12];
uint8_t frameBuff[12][XFER_BUFFER_LENGTH];
unsigned int frameCount;
};

#endif // DALY_BMS_UART_H