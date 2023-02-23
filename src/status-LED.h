/***
https://wokwi.com/projects/357439918883980289
*/


extern Daly_BMS_UART bms;
bool ledState = false;
void notificationLED(){

if(bms.get.chargeDischargeStatus == "offline")
{

}
 digitalWrite(LED_PIN, ledState);
}