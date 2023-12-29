/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
#include "daly.h"
SoftwareSerial myPort;

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
        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> ERROR: No serial peripheral specificed!");
        BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> ERROR: No serial peripheral specificed!");
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
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, min/max cell thresholds won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, min/max cell thresholds won't be modified!\n");
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
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, min/max pack voltage thresholds won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, min/max pack voltage thresholds won't be modified!\n");
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
        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Receive failed, V, I, & SOC values won't be modified!\n");
        BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Receive failed, V, I, & SOC values won't be modified!\n");
        clearGet();
        return false;
    }
    else
        // check if packCurrent in range
        if (((float)(((this->frameBuff[0][8] << 8) | this->frameBuff[0][9]) - 30000) / 10.0f) == -3000.f)
        {
            BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Receive failed, pack Current not in range. values won't be modified!\n");
            BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Receive failed, pack Current not in range. values won't be modified!\n");
            return false;
        }
        else
            // check if SOC in range
            if (((float)((this->frameBuff[0][10] << 8) | this->frameBuff[0][11]) / 10.0f) > 100.f)
            {
                BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Receive failed,SOC out of range. values won't be modified!\n");
                BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Receive failed,SOC out of range. values won't be modified!\n");
                return false;
            }

    // Pull the relevent values out of the buffer
    get.packVoltage = ((float)((this->frameBuff[0][4] << 8) | this->frameBuff[0][5]) / 10.0f);
    get.packCurrent = ((float)(((this->frameBuff[0][8] << 8) | this->frameBuff[0][9]) - 30000) / 10.0f);
    get.packSOC = ((float)((this->frameBuff[0][10] << 8) | this->frameBuff[0][11]) / 10.0f);
    //BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> " + (String)get.packVoltage + "V, " + (String)get.packCurrent + "A, " + (String)get.packSOC + "SOC");
    //BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> " + (String)get.packVoltage + "V, " + (String)get.packCurrent + "A, " + (String)get.packSOC + "SOC");
    return true;
}

bool DalyBms::getMinMaxCellVoltage() // 0x91
{
    if (!this->requestData(COMMAND::MIN_MAX_CELL_VOLTAGE, 1))
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, min/max cell values won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, min/max cell values won't be modified!\n");
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
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Temp values won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, Temp values won't be modified!\n");
        return false;
    }
    get.tempAverage = ((this->frameBuff[0][4] - 40) + (this->frameBuff[0][6] - 40)) / 2;

    return true;
}

bool DalyBms::getDischargeChargeMosStatus() // 0x93
{
    if (!this->requestData(COMMAND::DISCHARGE_CHARGE_MOS_STATUS, 1))
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Charge / discharge mos Status won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, Charge / discharge mos Status won't be modified!\n");
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
    get.resCapacitymAh = ((uint32_t)frameBuff[0][8] << 0x18) | ((uint32_t)frameBuff[0][9] << 0x10) | ((uint32_t)frameBuff[0][10] << 0x08) | (uint32_t)frameBuff[0][11];

    return true;
}

bool DalyBms::getStatusInfo() // 0x94
{
    if (!this->requestData(COMMAND::STATUS_INFO, 1))
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Status info won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, Status info won't be modified!\n");
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
                // BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Frame No.: " + (String)this->frameBuff[k][4]);
                // BMS_DEBUG_PRINTLN(" Cell No: " + (String)(cellNo + 1) + ". " + (String)((this->frameBuff[k][5 + i + i] << 8) | this->frameBuff[k][6 + i + i]) + "mV");
                // // BMS_DEBUG_WEB("<DALY-BMS DEBUG> Frame No.: " + (String)this->frameBuff[k][4]);
                // BMS_DEBUG_WEBLN(" Cell No: " + (String)(cellNo + 1) + ". " + (String)((this->frameBuff[k][5 + i + i] << 8) | this->frameBuff[k][6 + i + i]) + "mV");
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

    // for testing
    if (this->requestData(COMMAND::CELL_TEMPERATURE, 1))
    {
        for (size_t k = 0; k < ceil(get.numOfTempSensors / 7.0); k++)
        {
            for (size_t i = 0; i < 7; i++)
            {
                // BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Frame No.: ");
                // BMS_DEBUG_PRINT(this->frameBuff[k][4], DEC);
                // BMS_DEBUG_PRINT(" Sensor No: " + String(sensorNo + 1) + ". ");
                // BMS_DEBUG_PRINT(this->frameBuff[k][5 + i] - 40, DEC);
                // BMS_DEBUG_PRINTLN("C");
                // // BMS_DEBUG_WEB("<DALY-BMS DEBUG> Frame No.: ");
                // // BMS_DEBUG_WEB(this->frameBuff[k][4], DEC);
                // // BMS_DEBUG_WEB(" Sensor No: " + String(sensorNo + 1) + ". ");
                // // BMS_DEBUG_WEB(this->frameBuff[k][5 + i] - 40, DEC);
                // BMS_DEBUG_WEBLN("C");

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
        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Receive failed, Cell Balance State won't be modified!\n");
        BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Receive failed, Cell Balance State won't be modified!\n");
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

    // BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Cell Balance State: ");
     //BMS_DEBUG_WEB("<DALY-BMS DEBUG> Cell Balance State: ");
    // for (size_t i = 0; i < get.numberOfCells; i++)
    //{
    //     BMS_DEBUG_PRINT(get.cellBalanceState[i]);
    //     BMS_DEBUG_WEB(get.cellBalanceState[i]);
    // }
    // BMS_DEBUG_PRINTLN();
    // BMS_DEBUG_WEBLN();

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
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Failure Flags won't be modified!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Receive failed, Failure Flags won't be modified!\n");
        return false;
    }
    failCodeArr = "";
    /* 0x00 */
    // need renaming
    // https://github.com/all-solutions/DALY-docs-soft-firm/blob/main/docs/Daly%20UART_485%20Communications%20Protocol%20V1.2.pdf
    // level two is more important, check it first

    // alarm.levelOneCellVoltageTooHigh = bitRead(this->frameBuff[0][4], 0);
    // alarm.levelTwoCellVoltageTooHigh = bitRead(this->frameBuff[0][4], 1);
    if (bitRead(this->frameBuff[0][4], 1))
        failCodeArr += "Cell volt high level 2,";
    else if (bitRead(this->frameBuff[0][4], 0))
        failCodeArr += "Cell volt high level 1,";
    // alarm.levelOneCellVoltageTooLow = bitRead(this->frameBuff[0][4], 2);
    // alarm.levelTwoCellVoltageTooLow = bitRead(this->frameBuff[0][4], 3);
    if (bitRead(this->frameBuff[0][4], 3))
        failCodeArr += "Cell volt low level 2,";
    else if (bitRead(this->frameBuff[0][4], 2))
        failCodeArr += "Cell volt low level 1,";
    // alarm.levelOnePackVoltageTooHigh = bitRead(this->frameBuff[0][4], 4);
    // alarm.levelTwoPackVoltageTooHigh = bitRead(this->frameBuff[0][4], 5);
    if (bitRead(this->frameBuff[0][4], 5))
        failCodeArr += "Sum volt high level 2,";
    else if (bitRead(this->frameBuff[0][4], 4))
        failCodeArr += "Sum volt high level 1,";
    // alarm.levelOnePackVoltageTooLow = bitRead(this->frameBuff[0][4], 6);
    // alarm.levelTwoPackVoltageTooLow = bitRead(this->frameBuff[0][4], 7);
    if (bitRead(this->frameBuff[0][4], 7))
        failCodeArr += "Sum volt low level 2,";
    else if (bitRead(this->frameBuff[0][4], 6))
        failCodeArr += "Sum volt low level 1,";

    /* 0x01 */
    // alarm.levelOneChargeTempTooHigh = bitRead(this->frameBuff[0][5], 0);
    // alarm.levelTwoChargeTempTooHigh = bitRead(this->frameBuff[0][5], 1);
    if (bitRead(this->frameBuff[0][5], 1))
        failCodeArr += "Chg temp high level 2,";
    else if (bitRead(this->frameBuff[0][5], 0))
        failCodeArr += "Chg temp high level 1,";
    // alarm.levelOneChargeTempTooLow = bitRead(this->frameBuff[0][5], 2);
    //>alarm.levelTwoChargeTempTooLow = bitRead(this->frameBuff[0][5], 3);
    if (bitRead(this->frameBuff[0][5], 3))
        failCodeArr += "Chg temp low level 2,";
    else if (bitRead(this->frameBuff[0][5], 2))
        failCodeArr += "Chg temp low level 1,";
    // alarm.levelOneDischargeTempTooHigh = bitRead(this->frameBuff[0][5], 4);
    // alarm.levelTwoDischargeTempTooHigh = bitRead(this->frameBuff[0][5], 5);
    if (bitRead(this->frameBuff[0][5], 5))
        failCodeArr += "Dischg temp high level 2,";
    else if (bitRead(this->frameBuff[0][5], 4))
        failCodeArr += "Dischg temp high level 1,";
    // alarm.levelOneDischargeTempTooLow = bitRead(this->frameBuff[0][5], 6);
    // alarm.levelTwoDischargeTempTooLow = bitRead(this->frameBuff[0][5], 7);
    if (bitRead(this->frameBuff[0][5], 7))
        failCodeArr += "Dischg temp low level 2,";
    else if (bitRead(this->frameBuff[0][5], 6))
        failCodeArr += "Dischg temp low level 1,";
    /* 0x02 */
    // alarm.levelOneChargeCurrentTooHigh = bitRead(this->frameBuff[0][6], 0);
    // alarm.levelTwoChargeCurrentTooHigh = bitRead(this->frameBuff[0][6], 1);
    if (bitRead(this->frameBuff[0][6], 1))
        failCodeArr += "Chg overcurrent level 2,";
    else if (bitRead(this->frameBuff[0][6], 0))
        failCodeArr += "Chg overcurrent level 1,";
    // alarm.levelOneDischargeCurrentTooHigh = bitRead(this->frameBuff[0][6], 2);
    // alarm.levelTwoDischargeCurrentTooHigh = bitRead(this->frameBuff[0][6], 3);
    if (bitRead(this->frameBuff[0][6], 3))
        failCodeArr += "Dischg overcurrent level 2,";
    else if (bitRead(this->frameBuff[0][6], 2))
        failCodeArr += "Dischg overcurrent level 1,";
    // alarm.levelOneStateOfChargeTooHigh = bitRead(this->frameBuff[0][6], 4);
    // alarm.levelTwoStateOfChargeTooHigh = bitRead(this->frameBuff[0][6], 5);
    if (bitRead(this->frameBuff[0][6], 5))
        failCodeArr += "SOC high level 2,";
    else if (bitRead(this->frameBuff[0][6], 4))
        failCodeArr += "SOC high level 1,";
    // alarm.levelOneStateOfChargeTooLow = bitRead(this->frameBuff[0][6], 6);
    // alarm.levelTwoStateOfChargeTooLow = bitRead(this->frameBuff[0][6], 7);
    if (bitRead(this->frameBuff[0][6], 7))
        failCodeArr += "SOC Low level 2,";
    else if (bitRead(this->frameBuff[0][6], 6))
        failCodeArr += "SOC Low level 1,";

    /* 0x03 */
    // alarm.levelOneCellVoltageDifferenceTooHigh = bitRead(this->frameBuff[0][7], 0);
    // alarm.levelTwoCellVoltageDifferenceTooHigh = bitRead(this->frameBuff[0][7], 1);
    if (bitRead(this->frameBuff[0][7], 1))
        failCodeArr += "Diff volt level 2,";
    else if (bitRead(this->frameBuff[0][7], 0))
        failCodeArr += "Diff volt level 1,";
    // alarm.levelOneTempSensorDifferenceTooHigh = bitRead(this->frameBuff[0][7], 2);
    // alarm.levelTwoTempSensorDifferenceTooHigh = bitRead(this->frameBuff[0][7], 3);
    if (bitRead(this->frameBuff[0][7], 3))
        failCodeArr += "Diff temp level 2,";
    else if (bitRead(this->frameBuff[0][7], 2))
        failCodeArr += "Diff temp level 1,";
    /* 0x04 */
    // alarm.chargeFETTemperatureTooHigh = bitRead(this->frameBuff[0][8], 0);
    if (bitRead(this->frameBuff[0][8], 0))
        failCodeArr += "Chg MOS temp high alarm,";
    // alarm.dischargeFETTemperatureTooHigh = bitRead(this->frameBuff[0][8], 1);
    if (bitRead(this->frameBuff[0][8], 1))
        failCodeArr += "Dischg MOS temp high alarm,";
    // alarm.failureOfChargeFETTemperatureSensor = bitRead(this->frameBuff[0][8], 2);
    if (bitRead(this->frameBuff[0][8], 2))
        failCodeArr += "Chg MOS temp sensor err,";
    // alarm.failureOfDischargeFETTemperatureSensor = bitRead(this->frameBuff[0][8], 3);
    if (bitRead(this->frameBuff[0][8], 3))
        failCodeArr += "Dischg MOS temp sensor err,";
    // alarm.failureOfChargeFETAdhesion = bitRead(this->frameBuff[0][8], 4);
    if (bitRead(this->frameBuff[0][8], 4))
        failCodeArr += "Chg MOS adhesion err,";
    // alarm.failureOfDischargeFETAdhesion = bitRead(this->frameBuff[0][8], 5);
    if (bitRead(this->frameBuff[0][8], 5))
        failCodeArr += "Dischg MOS adhesion err,";
    // alarm.failureOfChargeFETTBreaker = bitRead(this->frameBuff[0][8], 6);
    if (bitRead(this->frameBuff[0][8], 6))
        failCodeArr += "Chg MOS open circuit err,";
    // alarm.failureOfDischargeFETBreaker = bitRead(this->frameBuff[0][8], 7);
    if (bitRead(this->frameBuff[0][8], 7))
        failCodeArr += " Discrg MOS open circuit err,";

    /* 0x05 */
    // alarm.failureOfAFEAcquisitionModule = bitRead(this->frameBuff[0][9], 0);
    if (bitRead(this->frameBuff[0][9], 0))
        failCodeArr += "AFE collect chip err,";
    // alarm.failureOfVoltageSensorModule = bitRead(this->frameBuff[0][9], 1);
    if (bitRead(this->frameBuff[0][9], 1))
        failCodeArr += "Voltage collect dropped,";
    // alarm.failureOfTemperatureSensorModule = bitRead(this->frameBuff[0][9], 2);
    if (bitRead(this->frameBuff[0][9], 2))
        failCodeArr += "Cell temp sensor err,";
    // alarm.failureOfEEPROMStorageModule = bitRead(this->frameBuff[0][9], 3);
    if (bitRead(this->frameBuff[0][9], 3))
        failCodeArr += "EEPROM err,";
    // alarm.failureOfRealtimeClockModule = bitRead(this->frameBuff[0][9], 4);
    if (bitRead(this->frameBuff[0][9], 4))
        failCodeArr += "RTC err,";
    // alarm.failureOfPrechargeModule = bitRead(this->frameBuff[0][9], 5);
    if (bitRead(this->frameBuff[0][9], 5))
        failCodeArr += "Precharge failure,";
    // alarm.failureOfVehicleCommunicationModule = bitRead(this->frameBuff[0][9], 6);
    if (bitRead(this->frameBuff[0][9], 6))
        failCodeArr += "Communication failure,";
    // alarm.failureOfIntranetCommunicationModule = bitRead(this->frameBuff[0][9], 7);
    if (bitRead(this->frameBuff[0][9], 7))
        failCodeArr += "Internal communication failure,";

    /* 0x06 */
    // alarm.failureOfCurrentSensorModule = bitRead(this->frameBuff[0][10], 0);
    if (bitRead(this->frameBuff[0][10], 0))
        failCodeArr += "Current module fault,";
    // alarm.failureOfMainVoltageSensorModule = bitRead(this->frameBuff[0][10], 1);
    if (bitRead(this->frameBuff[0][10], 1))
        failCodeArr += "Sum voltage detect fault,";
    // alarm.failureOfShortCircuitProtection = bitRead(this->frameBuff[0][10], 2);
    if (bitRead(this->frameBuff[0][10], 2))
        failCodeArr += "Short circuit protect fault,";
    // alarm.failureOfLowVoltageNoCharging = bitRead(this->frameBuff[0][10], 3);
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
    if (sw)
    {
        BMS_DEBUG_PRINTLN("Attempting to switch discharge MOSFETs on");
        BMS_DEBUG_WEBLN("Attempting to switch discharge MOSFETs on");
        // Set the first byte of the data payload to 1, indicating that we want to switch on the MOSFET
        requestCounter = 0;
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    else
    {
        BMS_DEBUG_PRINTLN("Attempting to switch discharge MOSFETs off");
        BMS_DEBUG_WEBLN("Attempting to switch discharge MOSFETs off");
        requestCounter = 0;
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> No response from BMS! Can't verify MOSFETs switched.\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> No response from BMS! Can't verify MOSFETs switched.\n");
        return false;
    }

    return true;
}

bool DalyBms::setChargeMOS(bool sw) // 0xDA 0x80 First Byte 0x01=ON 0x00=OFF
{
    if (sw == true)
    {
        BMS_DEBUG_PRINTLN("Attempting to switch charge MOSFETs on");
        BMS_DEBUG_WEBLN("Attempting to switch charge MOSFETs on");
        // Set the first byte of the data payload to 1, indicating that we want to switch on the MOSFET
        requestCounter = 0;
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::CHRG_FET);
    }
    else
    {
        BMS_DEBUG_PRINTLN("Attempting to switch charge MOSFETs off");
        BMS_DEBUG_WEBLN("Attempting to switch charge MOSFETs off");
        requestCounter = 0;
        this->sendCommand(COMMAND::CHRG_FET);
    }

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> No response from BMS! Can't verify MOSFETs switched.\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> No response from BMS! Can't verify MOSFETs switched.\n");
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
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Send failed, can't verify BMS was reset!\n");
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Send failed, can't verify BMS was reset!\n");
        return false;
    }

    return true;
}

bool DalyBms::setSOC(float val) // 0x21 last two byte is SOC
{
    if (val >= 0 && val <= 100)
    {
        requestCounter = 0;

        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Attempting to read the SOC");
        BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Attempting to read the SOC");
        // try read with 0x61
        this->sendCommand(COMMAND::READ_SOC);
        if (!this->receiveBytes())
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> 0x61 read failed");
            BMS_DEBUG_WEB("<DALY-BMS DEBUG> 0x61 read failed");
            // if 0x61 fails, write fake timestamp
            BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Attempting to set the SOC with fake RTC data");
            BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Attempting to set the SOC with fake RTC data");
            this->my_txBuffer[5] = 0x17; // year
            this->my_txBuffer[6] = 0x01; // month
            this->my_txBuffer[7] = 0x01; // day
            this->my_txBuffer[8] = 0x01; // hour
            this->my_txBuffer[9] = 0x01; // minute
        }
        else
        {
            BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Attempting to set the SOC with RTC data from BMS");
            BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Attempting to set the SOC with RTC data from BMS");
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
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> No response from BMS! Can't verify SOC.\n");
            BMS_DEBUG_WEB("<DALY-BMS DEBUG> No response from BMS! Can't verify SOC.\n");
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
        char debugBuff[128];
        sprintf(debugBuff, "<UART>[Command: 0x%2X][CRC Rec: %2X][CRC Calc: %2X]", cmdID, rxChecksum, this->frameBuff[i][XFER_BUFFER_LENGTH - 1]);
        //BMS_DEBUG_PRINTLN(debugBuff);
        //BMS_DEBUG_WEBLN(debugBuff);

        if (rxChecksum != this->frameBuff[i][XFER_BUFFER_LENGTH - 1])
        {
            BMS_DEBUG_PRINTLN("<UART> CRC FAIL");
            BMS_DEBUG_WEBLN("<UART> CRC FAIL");
            return false;
        }
        if (rxChecksum == 0)
        {
            BMS_DEBUG_PRINTLN("<UART> NO DATA");
            BMS_DEBUG_WEBLN("<UART> NO DATA");
            return false;
        }
        if (this->frameBuff[i][1] >= 0x20)
        {
            BMS_DEBUG_PRINTLN("<UART> BMS SLEEPING");
            BMS_DEBUG_WEBLN("<UART> BMS SLEEPING");
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
    BMS_DEBUG_PRINTLN();
    BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Command: 0x");
    BMS_DEBUG_PRINT(cmdID, HEX);
    BMS_DEBUG_PRINT(" CRC: 0x");
    BMS_DEBUG_PRINTLN(checksum, HEX);
    // BMS_DEBUG_WEBLN();
    BMS_DEBUG_WEB("<DALY-BMS DEBUG> Command: 0x");
    BMS_DEBUG_WEB(cmdID, HEX);
    BMS_DEBUG_WEB(" CRC: 0x");
    // BMS_DEBUG_WEBLN(checksum, HEX);

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
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Error: Received the wrong number of bytes! Expected 13, got ");
        BMS_DEBUG_PRINTLN(rxByteNum, DEC);
        BMS_DEBUG_WEB("<DALY-BMS DEBUG> Error: Received the wrong number of bytes! Expected 13, got ");
        // BMS_DEBUG_WEBLN(rxByteNum, DEC);
        this->barfRXBuffer();
        return false;
    }

    if (!validateChecksum())
    {
        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Error: Checksum failed!");
        // BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> Error: Checksum failed!");
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
    BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> CRC: Calc.: " + (String)checksum + " Rec.: " + (String)this->my_rxBuffer[XFER_BUFFER_LENGTH - 1]);
    // BMS_DEBUG_WEBLN("<DALY-BMS DEBUG> CRC: Calc.: " + (String)checksum + " Rec.: " + (String)this->my_rxBuffer[XFER_BUFFER_LENGTH - 1]);
    // Compare the calculated checksum to the real checksum (the last received byte)
    return (checksum == this->my_rxBuffer[XFER_BUFFER_LENGTH - 1]);
}

void DalyBms::barfRXBuffer(void)
{
    BMS_DEBUG_PRINT("<DALY-BMS DEBUG> RX Buffer: [");
    BMS_DEBUG_WEB("<DALY-BMS DEBUG> RX Buffer: [");
    for (int i = 0; i < XFER_BUFFER_LENGTH; i++)
    {
        BMS_DEBUG_PRINT(",0x" + (String)this->my_rxBuffer[i]);
        BMS_DEBUG_WEB(",0x" + (String)this->my_rxBuffer[i]);
    }
    BMS_DEBUG_PRINTLN("]");
    // BMS_DEBUG_WEBLN("]");
}

void DalyBms::clearGet(void)
{

    // data from 0x90
    // get.packVoltage = 0; // pressure (0.1 V)
    // get.packCurrent = 0; // acquisition (0.1 V)
    // get.packSOC = 0;     // State Of Charge

    // data from 0x91
    // get.maxCellmV = 0;   // maximum monomer voltage (mV)
    // get.maxCellVNum = 0; // Maximum Unit Voltage cell No.
    // get.minCellmV = 0;   // minimum monomer voltage (mV)
    // get.minCellVNum = 0; // Minimum Unit Voltage cell No.
    // get.cellDiff = 0;    // difference betwen cells

    // data from 0x92
    // get.tempAverage = 0; // Avergae Temperature

    // data from 0x93
    get.chargeDischargeStatus = "offline"; // charge/discharge status (0 stationary ,1 charge ,2 discharge)

    // get.chargeFetState = false;    // charging MOS tube status
    // get.disChargeFetState = false; // discharge MOS tube state
    // get.bmsHeartBeat = 0;          // BMS life(0~255 cycles)
    // get.resCapacitymAh = 0;        // residual capacity mAH

    // data from 0x94
    // get.numberOfCells = 0;                   // amount of cells
    // get.numOfTempSensors = 0;                // amount of temp sensors
    // get.chargeState = 0;                     // charger status 0=disconnected 1=connected
    // get.loadState = 0;                       // Load Status 0=disconnected 1=connected
    // memset(get.dIO, false, sizeof(get.dIO)); // No information about this
    // get.bmsCycles = 0;                       // charge / discharge cycles

    // data from 0x95
    // memset(get.cellVmV, 0, sizeof(get.cellVmV)); // Store Cell Voltages in mV

    // data from 0x96
    // memset(get.cellTemperature, 0, sizeof(get.cellTemperature)); // array of cell Temperature sensors

    // data from 0x97
    // memset(get.cellBalanceState, false, sizeof(get.cellBalanceState)); // bool array of cell balance states
    // get.cellBalanceActive = false;                                     // bool is cell balance active
}
