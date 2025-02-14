/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
#include "daly.h"
SoftwareSerial myPort;
extern void writeLog(const char* format, ...);
//----------------------------------------------------------------------
// Public Functions
//----------------------------------------------------------------------

DalyBms::DalyBms(int rx, int tx)
{
    soft_rx = rx;
    soft_tx = tx;
    this->my_serialIntf = &myPort;
}

bool DalyBms::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("<BMS > ERROR: No serial peripheral specificed!");
        get.connectionState = -3;
        return false;
    }

    // Initialize the serial link to 9600 baud with 8 data bits and no parity bits, per the Daly BMS spec
    this->my_serialIntf->begin(9600, SWSERIAL_8N1, soft_rx, soft_tx, false);

    memset(this->my_txBuffer, 0x00, XFER_BUFFER_LENGTH);
    clearGet();
    return true;
}

bool DalyBms::loop()
{
    if (millis() - previousTime >= DELAYTINME)
    {
        switch (requestCounter)
        {
        case 0:
            // requestCounter = sendCommand() ? (requestCounter + 1) : 0;
            requestCounter++;
            break;
        case 1:
            if (getPackMeasurements())
            {
                get.connectionState = true;
                errorCounter = 0;
                requestCounter++;
            }
            else
            {
                requestCounter = 0;
                if (errorCounter < ERRORCOUNTER)
                {
                    errorCounter++;
                }
                else
                {
                    get.connectionState = false;
                    errorCounter = 0;
                    requestCallback();
                    // clearGet();
                }
            }
            break;
        case 2:
            requestCounter = getMinMaxCellVoltage() ? (requestCounter + 1) : 0;
            break;
        case 3:
            requestCounter = getPackTemp() ? (requestCounter + 1) : 0;
            break;
        case 4:
            requestCounter = getDischargeChargeMosStatus() ? (requestCounter + 1) : 0;
            break;
        case 5:
            requestCounter = getStatusInfo() ? (requestCounter + 1) : 0;
            break;
        case 6:
            requestCounter = getCellVoltages() ? (requestCounter + 1) : 0;
            break;
        case 7:
            requestCounter = getCellTemperature() ? (requestCounter + 1) : 0;
            break;
        case 8:
            requestCounter = getCellBalanceState() ? (requestCounter + 1) : 0;
            break;
        case 9:
            requestCounter = getFailureCodes() ? (requestCounter + 1) : 0;
            if (getStaticData)
                requestCounter = 0;
            requestCallback();
            break;
        case 10:
            if (!getStaticData)
                requestCounter = getVoltageThreshold() ? (requestCounter + 1) : 0;
            requestCallback();
            break;
        case 11:
            if (!getStaticData)
                requestCounter = getPackVoltageThreshold() ? (requestCounter + 1) : 0;
            requestCounter = 0;
            requestCallback();
            getStaticData = true;
            break;

        default:
            break;
        }
        previousTime = millis();
    }
    return true;
}

bool DalyBms::getVoltageThreshold() // 0x59
{
    if (!this->requestData(COMMAND::CELL_THRESHOLDS, 1))
    {
        writeLog("<BMS > Receive failed %d", CELL_THRESHOLDS);
        return false;
    }

    get.maxCellThreshold1 = (float)((this->frameBuff[0][4] << 8) | this->frameBuff[0][5]);
    get.maxCellThreshold2 = (float)((this->frameBuff[0][6] << 8) | this->frameBuff[0][7]);
    get.minCellThreshold1 = (float)((this->frameBuff[0][8] << 8) | this->frameBuff[0][9]);
    get.minCellThreshold2 = (float)((this->frameBuff[0][10] << 8) | this->frameBuff[0][11]);

    return true;
}

bool DalyBms::getPackVoltageThreshold() // 0x5A
{
    if (!this->requestData(COMMAND::PACK_THRESHOLDS, 1))
    {
        writeLog("<BMS > Receive failed %d", PACK_THRESHOLDS);
        return false;
    }

    get.maxPackThreshold1 = (float)((this->frameBuff[0][4] << 8) | this->frameBuff[0][5]);
    get.maxPackThreshold2 = (float)((this->frameBuff[0][6] << 8) | this->frameBuff[0][7]);
    get.minPackThreshold1 = (float)((this->frameBuff[0][8] << 8) | this->frameBuff[0][9]);
    get.minPackThreshold2 = (float)((this->frameBuff[0][10] << 8) | this->frameBuff[0][11]);

    return true;
}

bool DalyBms::getPackMeasurements() // 0x90
{
    if (!this->requestData(COMMAND::VOUT_IOUT_SOC, 1))
    {
        writeLog("<BMS > Receive failed %d", VOUT_IOUT_SOC);
        clearGet();
        return false;
    }
    else
        // check if packCurrent in range
        if (((float)(((this->frameBuff[0][8] << 8) | this->frameBuff[0][9]) - 30000) / 10.0f) == -3000.f)
        {
            writeLog("<BMS > %d Cur. out of range", VOUT_IOUT_SOC);
            return false;
        }
        else
            // check if SOC in range
            if (((float)((this->frameBuff[0][10] << 8) | this->frameBuff[0][11]) / 10.0f) > 100.f)
            {
                writeLog("<BMS > %d SOC. out of range", VOUT_IOUT_SOC);
                return false;
            }

    // Pull the relevent values out of the buffer
    get.packVoltage = ((float)((this->frameBuff[0][4] << 8) | this->frameBuff[0][5]) / 10.0f);
    get.packCurrent = ((float)(((this->frameBuff[0][8] << 8) | this->frameBuff[0][9]) - 30000) / 10.0f);
    get.packSOC = ((float)((this->frameBuff[0][10] << 8) | this->frameBuff[0][11]) / 10.0f);
    return true;
}

bool DalyBms::getMinMaxCellVoltage() // 0x91
{
    if (!this->requestData(COMMAND::MIN_MAX_CELL_VOLTAGE, 1))
    {
        writeLog("<BMS > Receive failed %d", MIN_MAX_CELL_VOLTAGE);
        return false;
    }

    get.maxCellmV = (float)((this->frameBuff[0][4] << 8) | this->frameBuff[0][5]);
    get.maxCellVNum = this->frameBuff[0][6];
    get.minCellmV = (float)((this->frameBuff[0][7] << 8) | this->frameBuff[0][8]);
    get.minCellVNum = this->frameBuff[0][9];
    get.cellDiff = (get.maxCellmV - get.minCellmV);

    return true;
}

bool DalyBms::getPackTemp() // 0x92
{
    if (!this->requestData(COMMAND::MIN_MAX_TEMPERATURE, 1))
    {
        writeLog("<BMS > Receive failed %d", MIN_MAX_TEMPERATURE);
        return false;
    }
    get.tempAverage = ((this->frameBuff[0][4] - 40) + (this->frameBuff[0][6] - 40)) / 2;

    return true;
}

bool DalyBms::getDischargeChargeMosStatus() // 0x93
{
    if (!this->requestData(COMMAND::DISCHARGE_CHARGE_MOS_STATUS, 1))
    {
        writeLog("<BMS > Receive failed %d", DISCHARGE_CHARGE_MOS_STATUS);
        return false;
    }

    switch (this->frameBuff[0][4])
    {
    case 0:
        get.chargeDischargeStatus = "Stationary";
        break;
    case 1:
        get.chargeDischargeStatus = "Charge";
        break;
    case 2:
        get.chargeDischargeStatus = "Discharge";
        break;
    }

    get.chargeFetState = this->frameBuff[0][5];
    get.disChargeFetState = this->frameBuff[0][6];
    get.bmsHeartBeat = this->frameBuff[0][7];
    char msgbuff[16];
    float tmpAh = (((uint32_t)frameBuff[0][8] << 0x18) | ((uint32_t)frameBuff[0][9] << 0x10) | ((uint32_t)frameBuff[0][10] << 0x08) | (uint32_t)frameBuff[0][11]) * 0.001;
    dtostrf(tmpAh, 3, 1, msgbuff);
    get.resCapacityAh = atof(msgbuff);

    return true;
}

bool DalyBms::getStatusInfo() // 0x94
{
    if (!this->requestData(COMMAND::STATUS_INFO, 1))
    {
        writeLog("<BMS > Receive failed %d", STATUS_INFO);
        return false;
    }

    get.numberOfCells = this->frameBuff[0][4];
    get.numOfTempSensors = this->frameBuff[0][5];
    get.chargeState = this->frameBuff[0][6];
    get.loadState = this->frameBuff[0][7];

    // Parse the 8 bits into 8 booleans that represent the states of the Digital IO
    for (size_t i = 0; i < 8; i++)
    {
        get.dIO[i] = bitRead(this->frameBuff[0][8], i);
    }

    get.bmsCycles = ((uint16_t)this->frameBuff[0][9] << 0x08) | (uint16_t)this->frameBuff[0][10];

    return true;
}

bool DalyBms::getCellVoltages() // 0x95
{
    unsigned int cellNo = 0; // start with cell no. 1

    // Check to make sure we have a valid number of cells
    if (get.numberOfCells < MIN_NUMBER_CELLS && get.numberOfCells >= MAX_NUMBER_CELLS)
    {
        return false;
    }

    if (this->requestData(COMMAND::CELL_VOLTAGES, (unsigned int)ceil(get.numberOfCells / 3.0)))
    {
        for (size_t k = 0; k < (unsigned int)ceil(get.numberOfCells / 3.0); k++) // test for bug #67
        {
            for (size_t i = 0; i < 3; i++)
            {
                get.cellVmV[cellNo] = (this->frameBuff[k][5 + i + i] << 8) | this->frameBuff[k][6 + i + i];
                cellNo++;
                if (cellNo >= get.numberOfCells)
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

bool DalyBms::getCellTemperature() // 0x96
{
    unsigned int sensorNo = 0;
    // Check to make sure we have a valid number of temp sensors
    if ((get.numOfTempSensors < MIN_NUMBER_TEMP_SENSORS) && (get.numOfTempSensors >= MAX_NUMBER_TEMP_SENSORS))
    {
        return false;
    }

    if (this->requestData(COMMAND::CELL_TEMPERATURE, 1))
    {
        for (size_t k = 0; k < ceil(get.numOfTempSensors / 7.0); k++)
        {
            for (size_t i = 0; i < 7; i++)
            {
                get.cellTemperature[sensorNo] = (this->frameBuff[k][5 + i] - 40);
                sensorNo++;
                if (sensorNo >= get.numOfTempSensors)
                    break;
            }
        }
        return true;
    }
    else
    {
        writeLog("<BMS > Receive failed %d", CELL_TEMPERATURE);
        return false;
    }
}

bool DalyBms::getCellBalanceState() // 0x97
{
    int cellBalance = 0;
    int cellBit = 0;

    // Check to make sure we have a valid number of cells
    if (get.numberOfCells < MIN_NUMBER_CELLS && get.numberOfCells >= MAX_NUMBER_CELLS)
    {
        return false;
    }

    if (!this->requestData(COMMAND::CELL_BALANCE_STATE, 1))
    {
        writeLog("<BMS > Receive failed %d", CELL_BALANCE_STATE);
        return false;
    }

    // We expect 6 bytes response for this command
    for (size_t i = 0; i < 6; i++)
    {
        // For each bit in the byte, pull out the cell balance state boolean
        for (size_t j = 0; j < 8; j++)
        {
            get.cellBalanceState[cellBit] = bitRead(this->frameBuff[0][i + 4], j);
            cellBit++;
            if (bitRead(this->frameBuff[0][i + 4], j))
            {
                cellBalance++;
            }
            if (cellBit >= 47)
            {
                break;
            }
        }
    }

    if (cellBalance > 0)
    {
        get.cellBalanceActive = true;
    }
    else
    {
        get.cellBalanceActive = false;
    }

    return true;
}

bool DalyBms::getFailureCodes() // 0x98
{

    if (!this->requestData(COMMAND::FAILURE_CODES, 1))
    {
        writeLog("<BMS > Receive failed %d", FAILURE_CODES);
        return false;
    }
    failCodeArr = "";
    /* 0x00 */
    if (bitRead(this->frameBuff[0][4], 1))
        failCodeArr += "Cell volt high level 2,";
    else if (bitRead(this->frameBuff[0][4], 0))
        failCodeArr += "Cell volt high level 1,";
    if (bitRead(this->frameBuff[0][4], 3))
        failCodeArr += "Cell volt low level 2,";
    else if (bitRead(this->frameBuff[0][4], 2))
        failCodeArr += "Cell volt low level 1,";
    if (bitRead(this->frameBuff[0][4], 5))
        failCodeArr += "Sum volt high level 2,";
    else if (bitRead(this->frameBuff[0][4], 4))
        failCodeArr += "Sum volt high level 1,";
    if (bitRead(this->frameBuff[0][4], 7))
        failCodeArr += "Sum volt low level 2,";
    else if (bitRead(this->frameBuff[0][4], 6))
        failCodeArr += "Sum volt low level 1,";
    /* 0x01 */
    if (bitRead(this->frameBuff[0][5], 1))
        failCodeArr += "Chg temp high level 2,";
    else if (bitRead(this->frameBuff[0][5], 0))
        failCodeArr += "Chg temp high level 1,";
    if (bitRead(this->frameBuff[0][5], 3))
        failCodeArr += "Chg temp low level 2,";
    else if (bitRead(this->frameBuff[0][5], 2))
        failCodeArr += "Chg temp low level 1,";
    if (bitRead(this->frameBuff[0][5], 5))
        failCodeArr += "Dischg temp high level 2,";
    else if (bitRead(this->frameBuff[0][5], 4))
        failCodeArr += "Dischg temp high level 1,";
    if (bitRead(this->frameBuff[0][5], 7))
        failCodeArr += "Dischg temp low level 2,";
    else if (bitRead(this->frameBuff[0][5], 6))
        failCodeArr += "Dischg temp low level 1,";
    /* 0x02 */
    if (bitRead(this->frameBuff[0][6], 1))
        failCodeArr += "Chg overcurrent level 2,";
    else if (bitRead(this->frameBuff[0][6], 0))
        failCodeArr += "Chg overcurrent level 1,";
    if (bitRead(this->frameBuff[0][6], 3))
        failCodeArr += "Dischg overcurrent level 2,";
    else if (bitRead(this->frameBuff[0][6], 2))
        failCodeArr += "Dischg overcurrent level 1,";
    if (bitRead(this->frameBuff[0][6], 5))
        failCodeArr += "SOC high level 2,";
    else if (bitRead(this->frameBuff[0][6], 4))
        failCodeArr += "SOC high level 1,";
    if (bitRead(this->frameBuff[0][6], 7))
        failCodeArr += "SOC Low level 2,";
    else if (bitRead(this->frameBuff[0][6], 6))
        failCodeArr += "SOC Low level 1,";
    /* 0x03 */
    if (bitRead(this->frameBuff[0][7], 1))
        failCodeArr += "Diff volt level 2,";
    else if (bitRead(this->frameBuff[0][7], 0))
        failCodeArr += "Diff volt level 1,";
    if (bitRead(this->frameBuff[0][7], 3))
        failCodeArr += "Diff temp level 2,";
    else if (bitRead(this->frameBuff[0][7], 2))
        failCodeArr += "Diff temp level 1,";
    /* 0x04 */
    if (bitRead(this->frameBuff[0][8], 0))
        failCodeArr += "Chg MOS temp high alarm,";
    if (bitRead(this->frameBuff[0][8], 1))
        failCodeArr += "Dischg MOS temp high alarm,";
    if (bitRead(this->frameBuff[0][8], 2))
        failCodeArr += "Chg MOS temp sensor err,";
    if (bitRead(this->frameBuff[0][8], 3))
        failCodeArr += "Dischg MOS temp sensor err,";
    if (bitRead(this->frameBuff[0][8], 4))
        failCodeArr += "Chg MOS adhesion err,";
    if (bitRead(this->frameBuff[0][8], 5))
        failCodeArr += "Dischg MOS adhesion err,";
    if (bitRead(this->frameBuff[0][8], 6))
        failCodeArr += "Chg MOS open circuit err,";
    if (bitRead(this->frameBuff[0][8], 7))
        failCodeArr += " Discrg MOS open circuit err,";
    /* 0x05 */
    if (bitRead(this->frameBuff[0][9], 0))
        failCodeArr += "AFE collect chip err,";
    if (bitRead(this->frameBuff[0][9], 1))
        failCodeArr += "Voltage collect dropped,";
    if (bitRead(this->frameBuff[0][9], 2))
        failCodeArr += "Cell temp sensor err,";
    if (bitRead(this->frameBuff[0][9], 3))
        failCodeArr += "EEPROM err,";
    if (bitRead(this->frameBuff[0][9], 4))
        failCodeArr += "RTC err,";
    if (bitRead(this->frameBuff[0][9], 5))
        failCodeArr += "Precharge failure,";
    if (bitRead(this->frameBuff[0][9], 6))
        failCodeArr += "Communication failure,";
    if (bitRead(this->frameBuff[0][9], 7))
        failCodeArr += "Internal communication failure,";
    /* 0x06 */
    if (bitRead(this->frameBuff[0][10], 0))
        failCodeArr += "Current module fault,";
    if (bitRead(this->frameBuff[0][10], 1))
        failCodeArr += "Sum voltage detect fault,";
    if (bitRead(this->frameBuff[0][10], 2))
        failCodeArr += "Short circuit protect fault,";
    if (bitRead(this->frameBuff[0][10], 3))
        failCodeArr += "Low volt forbidden chg fault,";
    // remove the last character
    if (!failCodeArr.isEmpty())
    {
        failCodeArr.remove(failCodeArr.length() - 1, 1);
    }
    return true;
}

bool DalyBms::setDischargeMOS(bool sw) // 0xD9 0x80 First Byte 0x01=ON 0x00=OFF
{
    writeLog("<BMS > Switch discharge MOSFETs to %d", sw);
    if (sw)
    {
        // Set the first byte of the data payload to 1, indicating that we want to switch on the MOSFET
        requestCounter = 0;
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    else
    {
        requestCounter = 0;
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    if (!this->receiveBytes())
    {
        writeLog("<BMS > No response from BMS! Can't verify MOSFETs switched.");
        return false;
    }

    return true;
}

bool DalyBms::setChargeMOS(bool sw) // 0xDA 0x80 First Byte 0x01=ON 0x00=OFF
{
    writeLog("<BMS > Switch charge MOSFETs to %d", sw);
    if (sw)
    {
        // Set the first byte of the data payload to 1, indicating that we want to switch on the MOSFET
        requestCounter = 0;
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::CHRG_FET);
    }
    else
    {
        requestCounter = 0;
        this->sendCommand(COMMAND::CHRG_FET);
    }

    if (!this->receiveBytes())
    {
        writeLog("<BMS > No response from BMS! Can't verify MOSFETs switched.");
        return false;
    }

    return true;
}

bool DalyBms::setBmsReset() // 0x00 Reset the BMS
{
    requestCounter = 0;
    this->sendCommand(COMMAND::BMS_RESET);
    if (!this->receiveBytes())
    {
        writeLog("<BMS > Send failed, can't verify BMS was reset!");
        return false;
    }
    return true;
}

bool DalyBms::setSOC(float val) // 0x21 last two byte is SOC
{
    if (val >= 0 && val <= 100)
    {
        requestCounter = 0;

        writeLog("<BMS > Set SOC to %f", val);
        // try read with 0x61
        this->sendCommand(COMMAND::READ_SOC);
        if (!this->receiveBytes())
        {
            this->my_txBuffer[5] = 0x17; // year
            this->my_txBuffer[6] = 0x01; // month
            this->my_txBuffer[7] = 0x01; // day
            this->my_txBuffer[8] = 0x01; // hour
            this->my_txBuffer[9] = 0x01; // minute
        }
        else
        {
            for (size_t i = 5; i <= 9; i++)
            {
                this->my_txBuffer[i] = this->my_rxBuffer[i];
            }
        }
        uint16_t value = (val * 10);
        this->my_txBuffer[10] = (value & 0xFF00) >> 8;
        this->my_txBuffer[11] = (value & 0x00FF);
        this->sendCommand(COMMAND::SET_SOC);

        if (!this->receiveBytes())
        {
            writeLog("<BMS > Set SOC failed!");
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}

bool DalyBms::getState() // Function to return the state of connection
{
    return get.connectionState;
}

void DalyBms::callback(std::function<void()> func) // callback function when finnish request
{
    requestCallback = func;
}

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------

bool DalyBms::requestData(COMMAND cmdID, unsigned int frameAmount) // new function to request global data
{
    // Clear out the buffers
    memset(this->my_rxFrameBuffer, 0x00, sizeof(this->my_rxFrameBuffer));
    memset(this->frameBuff, 0x00, sizeof(this->frameBuff));
    memset(this->my_txBuffer, 0x00, XFER_BUFFER_LENGTH);
    //--------------send part--------------------
    uint8_t txChecksum = 0x00;    // transmit checksum buffer
    unsigned int byteCounter = 0; // bytecounter for incomming data
    // prepare the frame with static data and command ID
    this->my_txBuffer[0] = START_BYTE;
    this->my_txBuffer[1] = HOST_ADRESS;
    this->my_txBuffer[2] = cmdID;
    this->my_txBuffer[3] = FRAME_LENGTH;

    // Calculate the checksum
    for (uint8_t i = 0; i <= 11; i++)
    {
        txChecksum += this->my_txBuffer[i];
    }
    // put it on the frame
    this->my_txBuffer[12] = txChecksum;

    // send the packet
    this->my_serialIntf->write(this->my_txBuffer, XFER_BUFFER_LENGTH);
    // first wait for transmission end
    this->my_serialIntf->flush();
    //-------------------------------------------

    //-----------Recive Part---------------------
    /*uint8_t rxByteNum = */ this->my_serialIntf->readBytes(this->my_rxFrameBuffer, XFER_BUFFER_LENGTH * frameAmount);
    for (size_t i = 0; i < frameAmount; i++)
    {
        for (size_t j = 0; j < XFER_BUFFER_LENGTH; j++)
        {
            this->frameBuff[i][j] = this->my_rxFrameBuffer[byteCounter];
            byteCounter++;
        }

        uint8_t rxChecksum = 0x00;
        for (int k = 0; k < XFER_BUFFER_LENGTH - 1; k++)
        {
            rxChecksum += this->frameBuff[i][k];
        }
        //char debugBuff[128];
        //sprintf(debugBuff, "<UART>[Command: 0x%2X][CRC Rec: %2X][CRC Calc: %2X]", cmdID, rxChecksum, this->frameBuff[i][XFER_BUFFER_LENGTH - 1]);

        if (rxChecksum != this->frameBuff[i][XFER_BUFFER_LENGTH - 1])
        {
            writeLog("<BMS > CRC FAIL");
            return false;
        }
        if (rxChecksum == 0)
        {
            writeLog("<BMS > NO DATA");
            return false;
        }
        if (this->frameBuff[i][1] >= 0x20)
        {
            writeLog("<BMS > BMS SLEEPING");
            return false;
        }
    }
    return true;
}

bool DalyBms::sendQueueAdd(COMMAND cmdID)
{
    for (size_t i = 0; i < sizeof commandQueue / sizeof commandQueue[0]; i++) // run over the queue array
    {
        if (commandQueue[i] == 0x100) // search the next free slot for command
        {
            commandQueue[i] = cmdID; // put in the command
            break;
        }
    }
    return true;
}

bool DalyBms::sendCommand(COMMAND cmdID)
{
    uint8_t checksum = 0;
    do // clear all incoming serial to avoid data collision
    {
        char t __attribute__((unused)) = this->my_serialIntf->read(); // war auskommentiert, zum testen an

    } while (this->my_serialIntf->read() > 0);

    // prepare the frame with static data and command ID
    this->my_txBuffer[0] = START_BYTE;
    this->my_txBuffer[1] = HOST_ADRESS;
    this->my_txBuffer[2] = cmdID;
    this->my_txBuffer[3] = FRAME_LENGTH;

    // Calculate the checksum
    for (uint8_t i = 0; i <= 11; i++)
    {
        checksum += this->my_txBuffer[i];
    }
    // put it on the frame
    this->my_txBuffer[12] = checksum;

    this->my_serialIntf->write(this->my_txBuffer, XFER_BUFFER_LENGTH);
    // fix the sleep Bug
    // first wait for transmission end
    this->my_serialIntf->flush();

    // after send clear the transmit buffer
    memset(this->my_txBuffer, 0x00, XFER_BUFFER_LENGTH);
    requestCounter = 0; // reset the request queue that we get actual data
    return true;
}

bool DalyBms::receiveBytes(void)
{
    // Clear out the input buffer
    memset(this->my_rxBuffer, 0x00, XFER_BUFFER_LENGTH);
    memset(this->frameBuff, 0x00, sizeof(this->frameBuff));

    // Read bytes from the specified serial interface
    uint8_t rxByteNum = this->my_serialIntf->readBytes(this->my_rxBuffer, XFER_BUFFER_LENGTH);

    // Make sure we got the correct number of bytes
    if (rxByteNum != XFER_BUFFER_LENGTH)
    {
        writeLog("<BMS > Received wrong bytes! Expected 13, got %d", rxByteNum);
        this->barfRXBuffer();
        return false;
    }

    if (!validateChecksum())
    {
        writeLog("<BMS > Checksum failed!");
        this->barfRXBuffer();
        return false;
    }

    return true;
}

bool DalyBms::validateChecksum()
{
    uint8_t checksum = 0x00;

    for (int i = 0; i < XFER_BUFFER_LENGTH - 1; i++)
    {
        checksum += this->my_rxBuffer[i];
    }
    // Compare the calculated checksum to the real checksum (the last received byte)
    return (checksum == this->my_rxBuffer[XFER_BUFFER_LENGTH - 1]);
}

void DalyBms::barfRXBuffer(void)
{
    //BMS_DEBUG_PRINT("<DALY-BMS DEBUG> RX Buffer: [");
    //BMS_DEBUG_WEB("<DALY-BMS DEBUG> RX Buffer: [");
    for (int i = 0; i < XFER_BUFFER_LENGTH; i++)
    {
        //BMS_DEBUG_PRINT(",0x" + (String)this->my_rxBuffer[i]);
        //BMS_DEBUG_WEB(",0x" + (String)this->my_rxBuffer[i]);
    }
   // BMS_DEBUG_PRINTLN("]");
    // BMS_DEBUG_WEBLN("]");
}

void DalyBms::clearGet(void)
{
    get.chargeDischargeStatus = "offline"; // charge/discharge status (0 stationary ,1 charge ,2 discharge)
}
