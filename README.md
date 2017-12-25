# GSMRemote

*Work in progress!. Expected to be ready in early January 2018.*

A project that enable turning on and off relay just by calling a number. Main goals:
* Minimal power consumption.
* On and off just by calling GSM number.
* Stable operation for long period (over 50 days).

I started this project as a reliable way to power on the heater (Webasto) of my car. Remotes doesn't have sufficient range in some cases and some of them are qute expensive. The alternative is $10 of parts and an interesting challange.

## Hardware details
Required components:
* SIM800C module
* DC buck converter (or other efficient way to deliver over 2A current at ~3.8V)
* Arduino

*Wiring diagram to be included.*

## Programming the SIM800C

Several commands should be executed prior using the provided software.
* ATE0&W -> Set ECHO to OFF and save the settings.
* AT+CLIP=1 -> Set CLIP mode to 1. This will provide teh calling number on each RING.