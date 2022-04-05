#include "Arduino.h"
#include "daly-bms-uart.h"
// for debuggin
#define DALY_BMS_DEBUG
#define DEBUG_SERIAL Serial1

//----------------------------------------------------------------------
// Public Functions
//----------------------------------------------------------------------

Daly_BMS_UART::Daly_BMS_UART(HardwareSerial &serial_peripheral)
{
    this->my_serialIntf = &serial_peripheral;
}

bool Daly_BMS_UART::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.println("<DALY-BMS DEBUG> ERROR: No serial peripheral specificed!");
#endif
        return false;
    }

    // Intialize the serial link to 9600 baud with 8 data bits and no parity bits, per the Daly BMS spec
    this->my_serialIntf->begin(9600, SERIAL_8N1);

    // Set up the output buffer with some values that won't be changing
    this->my_txBuffer[0] = 0xA5; // Start byte
    this->my_txBuffer[1] = 0x40; // Host address
    // this->my_txBuffer[2] is where our command ID goes
    this->my_txBuffer[3] = 0x08; // Length?

    // Fill bytes 5-11 with 0s
    for (uint8_t i = 4; i < 12; i++)
    {
        this->my_txBuffer[i] = 0x00;
    }

    return true;
}

bool Daly_BMS_UART::update()
{

    getPackMeasurements();         // 0x90
    getMinMaxCellVoltage();        // 0x91
    getPackTemp();                 // 0x92
    getDischargeChargeMosStatus(); // 0x93
    getStatusInfo();               // 0x94
    getCellVoltages();             // 0x95 dont work, the answer string from BMS is too long and must splitet
    getFailureCodes();             // 0x98
    /**
     * put here the function call to recive all data one by one
     * check if cell number ar set and then ask for cell data
     *
     */
    return true;
}

bool Daly_BMS_UART::getPackMeasurements() // 0x90
{
    this->sendCommand(COMMAND::VOUT_IOUT_SOC);

    if (!this->receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, V, I, & SOC values won't be modified!\n");
#endif
        return false;
    }
    // Pull the relevent values out of the buffer
    get.packVoltage = (float)((this->my_rxBuffer[4] << 8) | this->my_rxBuffer[5]) / 10;
    // The current measurement is given with a 30000 unit offset
    get.packCurrent = (float)(((this->my_rxBuffer[8] << 8) | this->my_rxBuffer[9]) - 30000) / 10;
    get.packSOC = (float)((this->my_rxBuffer[10] << 8) | this->my_rxBuffer[11]) / 10;
    return true;
}

bool Daly_BMS_UART::getMinMaxCellVoltage() // 0x91
{
    this->sendCommand(COMMAND::MIN_MAX_CELL_VOLTAGE);

    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, min/max cell values won't be modified!\n");
#endif
        return false;
    }

    get.maxCellmV = (float)((this->my_rxBuffer[4] << 8) | this->my_rxBuffer[5]);
    get.maxCellVNum = this->my_rxBuffer[6];
    get.minCellmV = (float)((this->my_rxBuffer[7] << 8) | this->my_rxBuffer[8]);
    get.minCellVNum = this->my_rxBuffer[9];

    return true;
}

bool Daly_BMS_UART::getPackTemp() // 0x92
{
    this->sendCommand(COMMAND::MIN_MAX_TEMPERATURE);

    if (!this->receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, Temp value won't be modified!\n");
#endif
        return false;
    }

    uint8_t max_temp = (this->my_rxBuffer[4] - 40); // byte 0 from datasheet
    uint8_t min_temp = (this->my_rxBuffer[6] - 40); // byte 3 from datasheet
    get.tempAverage = (max_temp + min_temp) / 2;

    return true;
}

bool Daly_BMS_UART::getDischargeChargeMosStatus() // 0x93
{
    this->sendCommand(COMMAND::DISCHARGE_CHARGE_MOS_STATUS);

    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, Charge / discharge mos Status won't be modified!\n");
#endif
        return false;
    }
    get.chargeDischargeStatus = this->my_rxBuffer[4];
    get.chargeFetState = this->my_rxBuffer[5];
    get.disChargeFetState = this->my_rxBuffer[6];
    get.bmsHeartBeat = this->my_rxBuffer[7];
    get.resCapacitymAh = ((uint32_t)my_rxBuffer[8] << 0x18) | ((uint32_t)my_rxBuffer[9] << 0x10) | ((uint32_t)my_rxBuffer[10] << 0x08) | (uint32_t)my_rxBuffer[11];

    return true;
}

bool Daly_BMS_UART::getStatusInfo() // 0x94
{
    this->sendCommand(COMMAND::STATUS_INFO);

    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, Status info won't be modified!\n");
#endif
        return false;
    }
    get.numberOfCells = this->my_rxBuffer[4];
    get.numOfTempSensors = this->my_rxBuffer[5];
    get.chargeState = this->my_rxBuffer[6];
    get.loadState = this->my_rxBuffer[7];

    for (size_t i = 0; i < 8; i++)
    {
        get.dIO[i] = bitRead(this->my_rxBuffer[8], i);
    }
    get.bmsCycles = ((uint16_t)my_rxBuffer[9] << 0x08) | (uint16_t)my_rxBuffer[10];

    return true;
}

bool Daly_BMS_UART::getCellVoltages() // 0x95
{
    int cellNo = 0;

    if (get.numberOfCells > 1 && get.numberOfCells <= 48)
    {
        this->sendCommand(COMMAND::CELL_VOLTAGES);

        for (size_t i = 0; i <= ceil(get.numberOfCells / 3); i++)
        { // hier irgendwie runden, 2 bytes pro zelle, 3 zellen pro frame

            if (!receiveBytes())
            {
#ifdef DALY_BMS_DEBUG
                DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, Cell Voltages won't be modified!\n");
#endif
                break; // useless?
                return false;
            }

            for (size_t i = 0; i < 3; i++)
            {

#ifdef DALY_BMS_DEBUG
                DEBUG_SERIAL.print("<DALY-BMS DEBUG> Frame No.: " + (String)this->my_rxBuffer[4]);
                DEBUG_SERIAL.println(" Cell No: " + (String)(cellNo + 1) + ". " + (String)((this->my_rxBuffer[5 + i + i] << 8) | this->my_rxBuffer[6 + i + i]) + "mV");
#endif

                get.cellVmV[cellNo] = (this->my_rxBuffer[5 + i + i] << 8) | this->my_rxBuffer[6 + i + i];
                cellNo++;
                if (cellNo + 1 >= get.numberOfCells)
                    break;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Daly_BMS_UART::getFailureCodes() // 0x98
{
    this->sendCommand(COMMAND::FAILURE_CODES);

    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Receive failed, Failure Flags won't be modified!\n");
#endif
        return false;
    }
    /* 0x00 */
    alarm.levelOneCellVoltageTooHigh = bitRead(this->my_rxBuffer[4], 0);
    alarm.levelTwoCellVoltageTooHigh = bitRead(this->my_rxBuffer[4], 1);
    alarm.levelOneCellVoltageTooLow = bitRead(this->my_rxBuffer[4], 2);
    alarm.levelTwoCellVoltageTooLow = bitRead(this->my_rxBuffer[4], 3);
    alarm.levelOnePackVoltageTooHigh = bitRead(this->my_rxBuffer[4], 4);
    alarm.levelTwoPackVoltageTooHigh = bitRead(this->my_rxBuffer[4], 5);
    alarm.levelOnePackVoltageTooLow = bitRead(this->my_rxBuffer[4], 6);
    alarm.levelTwoPackVoltageTooLow = bitRead(this->my_rxBuffer[4], 7);

    /* 0x01 */
    alarm.levelOneChargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoChargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelOneChargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoChargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelOneDischargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoDischargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelOneDischargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoDischargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);

    /* 0x02 */
    alarm.levelOneChargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 0);
    alarm.levelTwoChargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 1);
    alarm.levelOneDischargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 2);
    alarm.levelTwoDischargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 3);
    alarm.levelOneStateOfChargeTooHigh = bitRead(this->my_rxBuffer[6], 4);
    alarm.levelTwoStateOfChargeTooHigh = bitRead(this->my_rxBuffer[6], 5);
    alarm.levelOneStateOfChargeTooLow = bitRead(this->my_rxBuffer[6], 6);
    alarm.levelTwoStateOfChargeTooLow = bitRead(this->my_rxBuffer[6], 7);

    /* 0x03 */
    alarm.levelOneCellVoltageDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 0);
    alarm.levelTwoCellVoltageDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 1);
    alarm.levelOneTempSensorDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 2);
    alarm.levelTwoTempSensorDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 3);

    /* 0x04 */
    alarm.chargeFETTemperatureTooHigh = bitRead(this->my_rxBuffer[8], 0);
    alarm.dischargeFETTemperatureTooHigh = bitRead(this->my_rxBuffer[8], 1);
    alarm.failureOfChargeFETTemperatureSensor = bitRead(this->my_rxBuffer[8], 2);
    alarm.failureOfDischargeFETTemperatureSensor = bitRead(this->my_rxBuffer[8], 3);
    alarm.failureOfChargeFETAdhesion = bitRead(this->my_rxBuffer[8], 4);
    alarm.failureOfDischargeFETAdhesion = bitRead(this->my_rxBuffer[8], 5);
    alarm.failureOfChargeFETTBreaker = bitRead(this->my_rxBuffer[8], 6);
    alarm.failureOfDischargeFETBreaker = bitRead(this->my_rxBuffer[8], 7);

    /* 0x05 */
    alarm.failureOfAFEAcquisitionModule = bitRead(this->my_rxBuffer[9], 0);
    alarm.failureOfVoltageSensorModule = bitRead(this->my_rxBuffer[9], 1);
    alarm.failureOfTemperatureSensorModule = bitRead(this->my_rxBuffer[9], 2);
    alarm.failureOfEEPROMStorageModule = bitRead(this->my_rxBuffer[9], 3);
    alarm.failureOfRealtimeClockModule = bitRead(this->my_rxBuffer[9], 4);
    alarm.failureOfPrechargeModule = bitRead(this->my_rxBuffer[9], 5);
    alarm.failureOfVehicleCommunicationModule = bitRead(this->my_rxBuffer[9], 6);
    alarm.failureOfIntranetCommunicationModule = bitRead(this->my_rxBuffer[9], 7);

    /* 0x06 */
    alarm.failureOfCurrentSensorModule = bitRead(this->my_rxBuffer[10], 0);
    alarm.failureOfMainVoltageSensorModule = bitRead(this->my_rxBuffer[10], 1);
    alarm.failureOfShortCircuitProtection = bitRead(this->my_rxBuffer[10], 2);
    alarm.failureOfLowVoltageNoCharging = bitRead(this->my_rxBuffer[10], 3);

    return true;
}

bool Daly_BMS_UART::setDischargeMOS(bool sw) // 0xD9 0x80 First Byte 0x01=ON 0x00=OFF
{
    if (sw == true)
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.println("try switching discharge on");
#endif
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::DISCHRG_FET);
        this->my_txBuffer[4] = 0x00;
    }

    else
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.println("try switching discharge off");
#endif
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> No response from BMS! Can't verify reset occurred.\n");
#endif
        return false;
    }

    return true;
}

bool Daly_BMS_UART::setChargeMOS(bool sw) // 0xDA 0x80 First Byte 0x01=ON 0x00=OFF
{
    if (sw == true)
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.println("try switching discharge on");
#endif
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::CHRG_FET);
        this->my_txBuffer[4] = 0x00;
    }

    else
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.println("try switching discharge off");
#endif
        this->sendCommand(COMMAND::CHRG_FET);
    }
    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> No response from BMS! Can't verify reset occurred.\n");
#endif
        return false;
    }

    return true;
}

bool Daly_BMS_UART::setBmsReset() // 0x00 Reset the BMS
{
    this->sendCommand(COMMAND::BMS_RESET);

    if (!receiveBytes())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Send failed, Discharge FET won't be modified!\n");
#endif
        return false;
    }

    return true;
}

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------

void Daly_BMS_UART::sendCommand(COMMAND cmdID)
{

    do //clear all incomming serial to avoid data collision
    {
        char t = this->my_serialIntf->read();
        t = 0;
    } while (this->my_serialIntf->read() > 0);

    uint8_t checksum = 0;
    this->my_txBuffer[2] = cmdID;
    // Calculate the checksum
    for (uint8_t i = 0; i <= 11; i++)
    {
        checksum += this->my_txBuffer[i];
    }
    // put it on the frame
    this->my_txBuffer[12] = checksum;

#ifdef DALY_BMS_DEBUG
    DEBUG_SERIAL.print("<DALY-BMS DEBUG> Checksum = 0x");
    DEBUG_SERIAL.println(checksum, HEX);
#endif

    this->my_serialIntf->write(this->my_txBuffer, XFER_BUFFER_LENGTH);
}

bool Daly_BMS_UART::receiveBytes(void)
{
    // Clear out the input buffer
    memset(this->my_rxBuffer, 0, XFER_BUFFER_LENGTH);

    // Read bytes from the specified serial interface
    uint8_t rxByteNum = this->my_serialIntf->readBytes(this->my_rxBuffer, XFER_BUFFER_LENGTH);

    // Make sure we got the correct number of bytes
    if (rxByteNum != XFER_BUFFER_LENGTH)
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.print("<DALY-BMS DEBUG> Error: Received the wrong number of bytes! Expected 13, got ");
        DEBUG_SERIAL.println(rxByteNum, DEC);
        this->barfRXBuffer();
#endif
        return false;
    }

    if (!validateChecksum())
    {
#ifdef DALY_BMS_DEBUG
        DEBUG_SERIAL.println("<DALY-BMS DEBUG> Error: Checksum failed!");
        this->barfRXBuffer();
#endif
        return false;
    }

    return true;
}

bool Daly_BMS_UART::validateChecksum()
{
    uint8_t checksum = 0x00;

    for (int i = 0; i < XFER_BUFFER_LENGTH - 1; i++)
    {
        checksum += this->my_rxBuffer[i];
    }

#ifdef DALY_BMS_DEBUG
    DEBUG_SERIAL.print("<DALY-BMS DEBUG> Calculated checksum: " + (String)checksum + ", Received checksum: " + (String)this->my_rxBuffer[XFER_BUFFER_LENGTH - 1] + "\n");
#endif

    // Compare the calculated checksum to the real checksum (the last received byte)
    return (checksum == this->my_rxBuffer[XFER_BUFFER_LENGTH - 1]);
}

void Daly_BMS_UART::barfRXBuffer(void)
{
#ifdef DALY_BMS_DEBUG
    DEBUG_SERIAL.print("<DALY-BMS DEBUG> RX Buffer: [");
    for (int i = 0; i < XFER_BUFFER_LENGTH; i++)
    {
        DEBUG_SERIAL.print("0x" + (String)this->my_rxBuffer[i]);
    }
    DEBUG_SERIAL.print("]\n");
#endif
}