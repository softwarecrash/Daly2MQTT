-------------------------------------------- Release Notes --------------------------------------------------

------------------------------------------------------ Interna ------------------------------------------------

99 little bugs in the code, 99 little bugs in the code. Take one down, patch it around 117 little bugs in the code.

Arbeiten:
[x] funktion für externe schaltausgänge fertigstellen
[x] funktion für wake fertigstellen, need blahblah? wake in mqtt einbauen

bestehende bugs:
[] decive control wenn über mqtt auf false, im web interface auf true, und dann wieder über mqtt false wird der wert nicht erneut gesendet.
[] soc setzen über mqtt nimmt er manchmal nicht, vermutlich weil grad eine abfrage durchläuft und die werte davon überschrieben werden
[] setzen der werte über mqtt zuverlässiger machen
[x] systemstart hängt wenn power von bms und aus schlaf wecken.

tests austehend:
[x] geht update vom letzten release ohne crash? wenn nicht minimal.bin bauen als übergang -> geht! (Daniel)


---------------------------------------------------ToDo next release------------------------------------------------

[_] redirect nach update optimieren, erst wenn esp wieder erreichbar oder fehlermeldung nach timeout
[_] code für set sleep time rausfischen
[_] eventuell tooltips in den settings hinzufügen
[_] SOC setzen mit nachkommastelle