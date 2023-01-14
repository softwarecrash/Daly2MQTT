-------------------------------------------- Release Notes --------------------------------------------------

V1.0.0

# !!!BREAKING CHANGES!!!
with this version a major change in data saving is coming up, befor update save your mqtt and time settings, it will be lost

New:
- Added SOC set over web
- Added function to use a external relais that is configurable using the webinterface on GPIO 14
- Relais can be set to manual mode so you can set it using mqtt or the switch on the web page
- Added function to use a external transistor to keep the bms awake on GPIO 12 (high active)
- Added confirmation dialog if you want to disable the discharging MOS to prevent you from you own personal blackout if you maybe miss-click

Bugfix:
- correct some little issues
- correcting OTA Update crash
- fix connection los bug [[#26](https://github.com/softwarecrash/DALY-BMS-to-MQTT/issues/26)]
- fix mqtt callback with empty payload

Changes:
- new eeprom saving variant
- updated all librarys
- some improvements with debug out
- reduce power consumption
- enable debug output for future use
- changed MQTT subscriptions to: /Device_Control/Relais, /Device_Control/Pack_SOC, /Device_Control/Pack_DischargeFET, /Device_Control/Pack_ChargeFET

------------------------------------------------------ Interna ------------------------------------------------

99 little bugs in the code, 99 little bugs in the code. Take one down, patch it around 117 little bugs in the code.

Arbeiten:
[x] funktion für externe schaltausgänge fertigstellen
[x] funktion für wake fertigstellen, need blahblah? wake in mqtt einbauen

bestehende bugs:
[x] systemstart hängt wenn power von bms und aus schlaf wecken.

tests austehend:
[x] geht update vom letzten release ohne crash? wenn nicht minimal.bin bauen als übergang -> geht! (Daniel)


---------------------------------------------------ToDo next release------------------------------------------------

[_] redirect nach update optimieren, erst wenn esp wieder erreichbar oder fehlermeldung nach timeout
[_] code für set sleep time rausfischen
[_] eventuell tooltips in den settings hinzufügen