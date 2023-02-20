release V2.x.x bitte drüber schauen und ggf formatieren

-----------------------------------------------------------------------------------------------
## WARNING - the new V2 requires hardware modifications

### New:
- Moved from hardwareserial to softwareserial, not transistor or isolator needed
- Support for ESP01
- LWT Alive Topic

### Changes:
- SOC now with float value
- Device_control subscribed topics now have callback for values
- Update all needed libraries to the last version

### Bugfix:
- [x] fix BMS crash or shutdown when set SOC in low range
- [x] fix webserver crashes
- [x] MQTT client ID fix - changed to unique
- [x] fix random reboot and hang up

Special Thanks for the support and hard work goes to

@derLoosi

@all-solutions

----------------------------------------------------------------------------------------------



# Release Notes

### New:
- added ESP VCC Warning
- added filter for weak wifi networks
- LWT Alive Topic

### Changes:
- SOC now with float value
- Device_control subscribed topics now have callback for values

### Bugfix:
- fix BMS crash or shutdown when set SOC in low Range
- fix Webserver crashes
- MQTT Client ID fix - changed to unique

## Interna

99 little bugs in the code, 99 little bugs in the code. Take one down, patch it around 117 little bugs in the code.

## ToDo

### Arbeiten
- [x] funktion für externe schaltausgänge fertigstellen
- [x] funktion für wake fertigstellen, wake in mqtt einbauen

### bestehende Bugs:
- [x] decive control wenn über mqtt auf false, im web interface auf true, und dann wieder über mqtt false wird der wert nicht erneut gesendet.
- [x] soc setzen über mqtt nimmt er manchmal nicht, vermutlich weil grad eine abfrage durchläuft und die werte davon überschrieben werden
- [x] setzen der werte über mqtt zuverlässiger machen
- [x] systemstart hängt wenn power von bms und aus schlaf wecken.
- [x] Relais im Manual Mode reagiert nicht ohne BMS Verbindung
- [x] Discharge-Mosfet schalter im Webinterface geht in manchen Browsern nicht? [Issue#49](https://github.com/softwarecrash/DALY-BMS-to-MQTT/issues/49) mehrfach getstet geht
- [x] Javascript so umbauen das die schalter nicht mehr springen falls möglich - bitte testen

- [x] edit config, when disabled output settings for relay, deactivate the other settings for it


### tests austehend:
- [x] geht update vom letzten release ohne crash? wenn nicht minimal.bin bauen als übergang -> geht! (Daniel)


### ToDo next release
- [ ] redirect nach update optimieren, erst wenn esp wieder erreichbar oder fehlermeldung nach timeout
- [x] code für set sleep time rausfischen - wird nicht mehr gebraucht, wake funktion übernimmt
- [ ] eventuell tooltips in den settings hinzufügen
- [x] SOC setzen mit nachkommastelle
- [x] Failsafe Option für Relais (verhalten bei verbindungsverlust zum BMS) !!! TESTEN !!!
- [x] Relais über WEB schalten (http://DEVICE IP/set?relais=1|0)
- [ ] custom mqtt topic mit custom payload für true|false vom relaisHandler als option (zB für WR Ladestrom auf 10A begrenzen bei über 99%SOC oder sowas ohne dass es über ein Script auf dem Broker geht)
- [ ] relaishandler schneller machen bei manual / mqtt mode - timeout ersetzen durch state machine
- [ ] BMS-Status "offline" per MQTT übertragen
- [x] alive Status einbauen mit lwt 

### Wünsche / Verbesserungen
- [ ] 3rd party: iobroker initiale werte für device_control senden wenn möglich [Issue#48](https://github.com/softwarecrash/DALY-BMS-to-MQTT/issues/48)
