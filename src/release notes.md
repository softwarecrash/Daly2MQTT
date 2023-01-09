!!!BREAKING CHANGES!!!
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

-------------------------------------------------------------------------------------------------------------------------------

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