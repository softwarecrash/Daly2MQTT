<h2>PCB for Wemos and BC327 OR ADUM1201</h2>

PCB for the Wemos with <b>either</b> the BC327 transistor or optionally the ADUM1201.
When using the BC237, the jumper "NO_ADUM" must be set.

As an additional option, the jumper "BMS-5V_TO_WEMOS" can be plugged in. With this, the Wemos is then completely supplied with power by the BMS. This depends on the type of BMS used (tested with an 8S 100A, HW version BMS-ST103-309E). 
<b>Never connect the Wemos via its own USB when the jumper is plugged in!</b>

![DALY-BMS-to-MQTT_Schematic](DALY-BMS-to-MQTT_Schematic.png) 

![DALY-BMS-to-MQTT_Board](DALY-BMS-to-MQTT_Board.png) 

![DALY-BMS-to-MQTT_TopSide](DALY-BMS-to-MQTT_TopSide.png) 

![DALY-BMS-to-MQTT_BottomSide](DALY-BMS-to-MQTT_BottomSide.png) 