# [Please vote for your favorite MQTT data style](https://forms.gle/SqKQsi3BrHDMyGC98)

# DALY-BMS-to-MQTT
Little Program for ESP82XX to get the Daly BMS data to web and MQTT

# Features:
- captive portal for wifi and MQTT config
- config in webinterface
- switching MOS gates over webinterface, MQTT and via web at /set?loadstate, set SOC over MQTT
- get essential data over webinterface, get all data like cell voltage and more over MQTT
- classic MQTT datapoints or Json string over MQTT
- get Json over web at /livejson?
- firmware update over webinterface
- debug log on D4 (9600 baud)
- wake the BMS over MQTT (only with external power supply) or keep it awake
- universal switching output



Based on daly-bms-uart, more info here:
https://github.com/softwarecrash/daly-bms-uart


Grab UART directly from the BMS and hook it up to the hardware serial


Main screen:

![grafik](https://user-images.githubusercontent.com/44615614/212401798-0ced966d-4549-4958-af7f-98ceed967128.png)


Settings:

![grafik](https://user-images.githubusercontent.com/44615614/212401754-81a16130-f24d-4c8a-babc-d18d112fad5a.png)

Config:

![grafik](https://user-images.githubusercontent.com/44615614/212401591-dadfd5c6-6b0f-42f4-8ab0-44efc8c37553.png)

MQTT Data

![grafik](https://user-images.githubusercontent.com/44615614/161782578-aabdde4d-4f51-4312-9392-9fdf4d45df24.png)

# Connection to BMS:

Normally you don't need extra hardware, but various models of the BMS don't have enough power to pull down the rx pin from the ESP. In this case you need an amplifier like a BC327-25 to pull down the voltage, so that the communication works.

![image](https://user-images.githubusercontent.com/17761850/212558306-40de7d88-bad4-4ae8-9cb6-76db57cd419a.png)

Pin D5 of the Wemos is for the wake. Please connect according to wiring, **NEVER** connect the pin directly to the BMS!

**If you supply the Wemos externally with power: NEVER use for example 5V output of the battery or similar. ALWAYS use a separate USB power supply (cell phone charger). Otherwise there is a risk that the Wemos will be destroyed!**

**We assume no warranty and / or recourse for destroyed devices.**

# How to use:
- flash the bin file to an ESP82xx or Wemos D1 Mini with Tasmotizer (https://github.com/tasmota/tasmotizer/releases) or a similar program
- connect the ESP like the wiring diagram
- search for the wifi ap "DALY-BMS-AP" and connect to it
- surf to 192.168.4.1 and set up your wifi and optional MQTT
- that's it :)

# For external wiring please take a look at the Wiki!


Here you can find the communication methods for your BMS:
https://www.dalyelec.cn/newsshow.php?cid=24&id=65&lang=1


Questions? Join https://discord.gg/HsYjT7eXQW (German / English)

# Known Bugs:
- a small batch of Daly BMS don't work since V0.3.9. A workaround is to enable the debug output in the daly-bms-uart library. It's patched with versions higher than V0.4.14




