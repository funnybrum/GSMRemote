# GSMRemote

*Work in progress!. Expected to be finished in early January 2018.*

I started this project as a reliable way to power on the heater (Webasto) of my car. Remotes doesn't have sufficient range in some cases and some of them are qute expensive. The alternative is $10 of parts and an interesting challange. Main goals are:
* Minimal power consumption, let's not drain the battery for several weeks.
* On and off just by calling GSM number. This is free, SMS is not.
* Stable operation for long period (over 50 days). I.e. - not affected by internal timer rollover, no spontanions resets.
* Automatic power off if the car engine is started up. The car VCC will get over 13V if the engine is running. If this happens - power off the heater.


## Hardware details
Required components:
* SIM800C module
* DC buck converter (or other efficient way to deliver over 2A current at ~3.8V)
* Arduino
* 12V relay, a few diodes, transistors and resistors.

## Wiring diagram

Some parts are still missing. Like the rellay control circuit and the VCC following circuit.

![GSM Remote wiring diagram](./schematic/gsm_remote.png "Wiring diagram")


## Programming the SIM800C

Several commands should be executed prior using the provided source code.
* ATE0&W -> Set ECHO to OFF and save the settings.
* AT+CLIP=1 -> Set CLIP mode to 1. This will provide teh calling number on each RING.