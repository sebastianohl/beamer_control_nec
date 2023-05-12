# Homie beamer control for NEC rs232 interface 

Needed Hardware:
* EPS8266 NodeMCU
* RS232-C level shifter board (e.g. MAX3232)

Configuration:
* use idy.py menuconfig to set parameters (e.g. WiFi Password, MQTT Settings...)

Software:
* vscode with remote containers extension 

Important:
* your NEC P525UL beamer will not go into deep sleep if it is continously asked for its power status. 
  It will then remain in SLEEP mode regardless of your STANDBY_MODE setting. Therefore, this software will
  stop asking for the beamer status for 60s to allow deep sleep mode.

NEC Beamer Communication:
* Binary Format (this is used) [https://www.sharpnecdisplays.eu/p/download/cp/Products/Projectors/Shared/CommandLists/PDF-ExternalControlManual-english.pdf?fn=ExternalControlManual-english.pdf]
* ASCII Format (this will not wake up your beamer from deep sleep) [https://www.sharp-nec-displays.com/support/webdl/dl_service/data/sl/en/command/common_ascii_e-r1.pdf]
