# Homie beamer control for NEC rs232 interface 

Needed Hardware:
* EPS8266 NodeMCU

Configuration:
* use idy.py menuconfig to set parameters (e.g. WiFi Password, MQTT Settings...)

Software:
* vscode with remote containers extension 

Important:
* set the correct standby mode or your beamer will not wake up because the RS232 is also on sleep. For P525UL the mode is called "network standby"

NEC Beamer Communication:
* Binary Format [https://www.sharpnecdisplays.eu/p/download/cp/Products/Projectors/Shared/CommandLists/PDF-ExternalControlManual-english.pdf?fn=ExternalControlManual-english.pdf]
* ASCII Format (this is used) [https://www.sharp-nec-displays.com/support/webdl/dl_service/data/sl/en/command/common_ascii_e-r1.pdf]
