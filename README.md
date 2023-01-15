# [Please vote your favorite mqtt data style](https://forms.gle/SqKQsi3BrHDMyGC98)

# DALY-BMS-to-MQTT
Little Program for ESP82XX to get the Daly BMS Data to web and MQTT

# Features:
- Captive Portal for Wlan and mqtt config
- config in webinterface
- Switching MOS Gates over Webinterface, MQTT and via web at /set?loadstate, set SOC over mqtt
- Get essential Data over Webinterface, all Data Like Cell voltage and more get over MQTT
- classic MQTT Datapoints or Json String over MQTT
- get Json over web at /livejson?
- firmware update over webinterface
- Debug log on D4 (9600 baud)
- Wake the bms over mqtt or hold it wake
- universal Switching output

# Known Bugs:
- a smal batch of daly BMS dont work since V0.3.9 a workaround is to enable the debug output in the daly-bms-uart library. it is patched with versions higher than V0.4.14




Based on this work, more info here:
https://github.com/softwarecrash/daly-bms-uart


grab UART directly from the BMS and hook it up to the Hardware Serial


Main screen:

![grafik](https://user-images.githubusercontent.com/44615614/212401798-0ced966d-4549-4958-af7f-98ceed967128.png)


Settings:

![grafik](https://user-images.githubusercontent.com/44615614/212401754-81a16130-f24d-4c8a-babc-d18d112fad5a.png)

Config:

![grafik](https://user-images.githubusercontent.com/44615614/212401591-dadfd5c6-6b0f-42f4-8ab0-44efc8c37553.png)

MQTT Data

![grafik](https://user-images.githubusercontent.com/44615614/161782578-aabdde4d-4f51-4312-9392-9fdf4d45df24.png)

Connection to BMS
normal you dont neet the ADUM1201, but varius models of the BMS have not enugh power to pull down the rx pin from the esp, in this case you need a amplifier like a BC327-25 or the ADUM1201 to pull down ne voltage that the communication works.

![182551990-30c1826e-b988-4045-84b5-a2bfb602262b](https://user-images.githubusercontent.com/44615614/193560735-5d6c40cd-412c-4e0d-b6e7-bd906d383daa.png)

![178109199-b927b9e7-a20c-447c-9c8d-69dbe1a4f549](https://user-images.githubusercontent.com/44615614/193560745-a6431f65-6359-46f3-aa21-dc08c978dffb.png)


# How to use:
- flash the bin file to a esp82xx or Wemos D1 Mini with tasmotizer or other way
- connect the esp like the wireing diagram
- search the wifi ap DALY-BMS-AP and connect
- surf to 192.168.4.1 and set up your wifi and optional mqtt
- thats it :)

# For external wireing please take a lok at the Wiki!


here you can find the communications methods for your bms
https://www.dalyelec.cn/newsshow.php?cid=24&id=65&lang=1


Questions? join https://discord.gg/HsYjT7eXQW (German / English)
