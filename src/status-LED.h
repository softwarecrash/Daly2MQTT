/***
https://wokwi.com/projects/357439918883980289
*/


extern Daly_BMS_UART bms;
bool ledState = false;
void notificationLED(){

if(strcmp(bms.get.chargeDischargeStatus, "offline") == 0)
{

}
 digitalWrite(LED_PIN, ledState);
}