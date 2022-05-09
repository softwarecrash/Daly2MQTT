# DALY-BMS-to-MQTT
Little Program to get the Daly BMS Data to web and MQTT

# Features:
- Captive Portal for Wlan and mqtt config
- Switching MOS Gates over Webinterface and MQTT
- Get essential Data over Webinterface, all Data Like Cell voltage and more get over MQTT
- Debug log on D4 (9600 baud)


Based on this work, more info here:
https://github.com/softwarecrash/daly-bms-uart


grab UART directly from the BMS and hook it up to the Hardware Serial


Main screen:

![grafik](https://user-images.githubusercontent.com/44615614/162031230-e974bd8d-6201-4733-9c5d-2bd9b63daede.png)


Settings:

![grafik](https://user-images.githubusercontent.com/44615614/161764632-6a4ec457-971b-418e-b520-6933797cdff0.png)

MQTT and Name Config:

![grafik](https://user-images.githubusercontent.com/44615614/161764827-db9a57db-34c8-4b62-857a-759bba5c46aa.png)

MQTT Data

![grafik](https://user-images.githubusercontent.com/44615614/161782578-aabdde4d-4f51-4312-9392-9fdf4d45df24.png)

Connection to BMS
normal you dont neet the transistor in the rx line, but varius models of the BMS have not enugh power to pull down the rx pin from the esp, in this case you need a amplifier like the transistor to pull down ne voltage that the communication works.

![daly_bms_pinout](https://user-images.githubusercontent.com/44615614/163577322-3587ad81-070b-4115-b7e6-3f1c50a6a563.png)

here you can find the communications methods for your bms
https://www.dalyelec.cn/newsshow.php?cid=24&id=65&lang=1


Like my Work? Buy me a Coffee https://paypal.me/tobirocky
