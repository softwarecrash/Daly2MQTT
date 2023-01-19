-------------------------------------------- Release Notes --------------------------------------------------

------------------------------------------------------ Interna ------------------------------------------------

99 little bugs in the code, 99 little bugs in the code. Take one down, patch it around 117 little bugs in the code.

Arbeiten:
[x] funktion für externe schaltausgänge fertigstellen
[x] funktion für wake fertigstellen, need blahblah? wake in mqtt einbauen

bestehende bugs:
[_] decive control wenn über mqtt auf false, im web interface auf true, und dann wieder über mqtt false wird der wert nicht erneut gesendet.
[_] soc setzen über mqtt nimmt er manchmal nicht, vermutlich weil grad eine abfrage durchläuft und die werte davon überschrieben werden
[_] setzen der werte über mqtt zuverlässiger machen
[x] systemstart hängt wenn power von bms und aus schlaf wecken.


tests austehend:
[x] geht update vom letzten release ohne crash? wenn nicht minimal.bin bauen als übergang -> geht! (Daniel)


---------------------------------------------------ToDo next release------------------------------------------------

[_] redirect nach update optimieren, erst wenn esp wieder erreichbar oder fehlermeldung nach timeout
[_] code für set sleep time rausfischen
[_] eventuell tooltips in den settings hinzufügen
[_] SOC setzen mit nachkommastelle
[_] option to keep last relais state on bms connection loss / failsafe

-----------------------------------------------Wünsche / Verbesserungen --------------------------------------------

[_] 3rd party: iobroker initiale werte für device_control senden wenn möglich https://github.com/softwarecrash/DALY-BMS-to-MQTT/issues/48
