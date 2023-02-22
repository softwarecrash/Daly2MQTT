# [Please vote for your favorite MQTT data style](https://forms.gle/SqKQsi3BrHDMyGC98)

# DALY-BMS-to-MQTT
Little Program for ESP82XX to get the Daly BMS data to web and MQTT

# Features:
- introducing support for ESP-01 (with limited features)
- captive portal for wifi and MQTT config
- config in webinterface
- switching MOS gates over webinterface, MQTT and via web at /set?loadstate, set SOC over MQTT
- get essential data over webinterface, get all data like cell voltage and more over MQTT
- classic MQTT datapoints or Json string over MQTT
- get Json over web at /livejson?
- firmware update over webinterface
- debug log on Wemos USB (use only if you **don't** supply Wemos from BMS!!!)
- wake the BMS over MQTT or keep it awake (not supported on ESP-01)
- universal switching output (only with external power supply, not supported on ESP-01)



Based on daly-bms-uart, more info here:
https://github.com/softwarecrash/daly-bms-uart


Grab UART directly from the BMS and hook it up to the hardware serial


Main screen:

![grafik](https://user-images.githubusercontent.com/44615614/212401798-0ced966d-4549-4958-af7f-98ceed967128.png)


Settings:

![grafik](https://user-images.githubusercontent.com/44615614/212401754-81a16130-f24d-4c8a-babc-d18d112fad5a.png)

Config:

![grafik](https://user-images.githubusercontent.com/17761850/214363585-8deedb92-4947-46a8-97bf-9a069bc4b8fe.png)

MQTT Data

![grafik](https://user-images.githubusercontent.com/44615614/161782578-aabdde4d-4f51-4312-9392-9fdf4d45df24.png)

# Connection to BMS:

Normally you don't need extra hardware, but various models of the BMS don't have enough power to pull down the rx pin from the ESP. In this case you need an amplifier like a BC327-25 (or -40) to pull down the voltage, so that the communication works.

**With the new v2, no additional hardware (BC327 or ADUM) is required for communication.**

**Due to the number of different possible connections in the meantime, all connection diagrams have been moved to the [Wiki](https://github.com/softwarecrash/DALY-BMS-to-MQTT/wiki/Output-Connections). There you can see all connection diagrams that are currently available.**

# How to use:
- flash the bin file to an ESP8266 (recommended Wemos D1 Mini) with [Tasmotizer](https://github.com/tasmota/tasmotizer/releases) or use the [Online Flash tool](https://softwarecrash.github.io/DALY-BMS-to-MQTT/espflashtool/)
- connect the ESP like the wiring diagram
- search for the wifi ap "DALY-BMS-AP" and connect to it
- surf to 192.168.4.1 and set up your wifi and optional MQTT
- that's it :)

# For external wiring please take a look at the [Wiki](https://github.com/softwarecrash/DALY-BMS-to-MQTT/wiki/Output-Connections)!


Here you can find the communication methods for your BMS:
https://www.dalyelec.cn/newsshow.php?cid=24&id=65&lang=1


Questions? Join https://discord.gg/HsYjT7eXQW (German / English)

# Known Bugs:
- a small batch of Daly BMS don't work since V0.3.9. A workaround is to enable the debug output in the daly-bms-uart library. It's patched with versions higher than V0.4.14



