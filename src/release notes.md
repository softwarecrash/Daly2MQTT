-------------------------------------------- Release Notes --------------------------------------------------



# !!!BREAKING CHANGES!!!
with this version a major change in data saving is coming up, befor update save your mqtt and time settings, it will be lost

New:
- Added SOC set over web
- Added external relais function
- 

Bugfix:
- correct some little issues
- correcting OTA space bug

Changes:
- new eeprom saving variant
- updated all librarys
- some improvements with debug out
- reduce power consumption
- enable debug output for future use

------------------------------------------------------ Interna ------------------------------------------------

99 little bugs in the code, 99 little bugs in the code. Take one down, patch it around 117 little bugs in the code.

Arbeiten:
- funktion für externe schaltausgänge fertigstellen
- funktion für wake fertigstellen, need blahblah? wake in mqtt einbauen
- code für set sleep time rausfischen
- noch irgendwas

bestehende bugs:
- systemstart hängt wenn power von bms und aus schlaf wecken.

tests austehend:
- geht update vom letzten release ohne crash? wenn nicht minimal.bin bauen als übergang

sonstiges:
- versionssprung? eigentlich wäre es nach version semantic ein major relase was dann v 1.0.0 darstellen würde, oder man nimmt an das ein minor ist dann wäre man bei v0.5.xx
