#ifndef DALY_BMS_UART_H
#define DALY_BMS_UART_H

#define XFER_BUFFER_LENGTH 13

class Daly_BMS_UART
{
public:
    // These command names are adapted/translated from the protocol description (see /docs/)
    // Note: Not all are currently supported with public functions
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
        BMS_RESET = 0x00,  //Reseting the BMS
    };
    struct
    {
        //data from 0x90
        float packVoltage;          //pressure (0.1 V) 
        float packCurrent;          //acquisition (0.1 V) 
        float packSOC;              //State Of Charge
        
        //data from 0x91
        float maxCellmV;            //maximum monomer voltage (mV) 
        int maxCellVNum;            //Maximum Unit Voltage cell No.
        float minCellmV;            //minimum monomer voltage (mV) 
        int minCellVNum;            //Minimum Unit Voltage cell No.
        float cellDiff;             //difference betwen cells

        //data from 0x92
        int tempMax;              // maximum monomer temperature (40 Offset,°C)
        int tempMin;              //Maximum monomer temperature cell No.
        float tempAverage;          //Avergae Temperature

        //data from 0x93
        String chargeDischargeStatus;  //charge/discharge status (0 stationary ,1 charge ,2 discharge)
        bool chargeFetState;        //charging MOS tube status
        bool disChargeFetState;     //discharge MOS tube state
        int bmsHeartBeat;           //BMS life(0~255 cycles) 
        int resCapacitymAh;         //residual capacity mAH

        //data from 0x94
        int numberOfCells;          //amount of cells
        int numOfTempSensors;       //amount of temp sensors
        bool chargeState;           //charger status 0=disconnected 1=connected
        bool loadState;             //Load Status 0=disconnected 1=connected
        bool dIO[8];                //No information about this
        int bmsCycles;              //charge / discharge cycles
        
        //data from 0x95
        float cellVmV[48];          //Store Cell Voltages in mV

        //data from 0x96
        int cellTemperature[16];

        //data from 0x97
        bool cellBalanceState[48];
        bool cellBalanceActive;
    } get;

    struct
    {
        //data from 0x98
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

    Daly_BMS_UART(HardwareSerial &serialIntf);

    /**
     * @brief Initializes this driver
     * @details Configures the serial peripheral and pre-loads the transmit buffer with command-independent bytes
     */
    bool Init();

    /**
     * send data to the bms and update the values
     */
    bool update();

    /**
     * @brief Gets Voltage, Current, and SOC measurements from the BMS
     * @return True on successful aquisition, false otherwise
     */
    bool getPackMeasurements();

    /**
     * @brief Gets the pack temperature in degrees celsius
     * @details This function uses the MIN_MAX_TEMPERATURE command, and averages the 
     * min and max temperatures to get the returned value
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
    bool setDischargeMOS (bool sw);

    /**
     * @brief set the Charging MOS State
     * 
     */
    bool setChargeMOS (bool sw);

    /**
     * @brief Read the charge and discharge MOS States
     * 
     */
    bool getDischargeChargeMosStatus();

    /**
     * @brief Reseting The BMS
     * 
     */
    bool setBmsReset();
private:
    /**
     * @brief Sends a complete packet with the specified command
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    void sendCommand(COMMAND cmdID);

    /**
     * @brief 
     * @details
     * @return True on success, false on failure
     */
    bool receiveBytes(void);

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
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    HardwareSerial *my_serialIntf;

    /**
     * @brief Buffer used to transmit data to the BMS
     * @details Populated primarily in the "Init()" function, see the readme for more info
     */
    uint8_t my_txBuffer[XFER_BUFFER_LENGTH];

    /**
     * @brief Buffer filled with data from the BMS
     */
    uint8_t my_rxBuffer[XFER_BUFFER_LENGTH];
};

#endif // DALY_BMS_UART_H