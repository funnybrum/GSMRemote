# GSMRemote

*Work in progress!. Expected to be finished in early January 2018.*

I started this project as a reliable way to power on the heater (a Webasto one) of my car. Remotes doesn't have sufficient range in some cases and some of them are qute expensive. The alternative is $10 of parts and an interesting challange. Main goals are:
* Minimal power consumption, let's not drain the battery for several weeks.
* On and off just by calling GSM number. This is free, SMS is not.
* Feedback if the call was received. A simple one would be to hang up the call.
* Stable operation for long period (over 50 days). I.e. - not affected by internal timer rollover, no spontanions resets.
* Automatic power off if the car engine is started up. The car VCC will get over 13V if the engine is running. If this happens - power off the heater.

## Hardware details
Required components:
* SIM800C module
* DC buck converter (or other efficient way to deliver over 2A current at ~3.8V)
* Arduino
* 12V relay, a few diodes, transistors and resistors.

## Wiring diagram

Still not completely tested. The VBAT measruing circuit may be modified, the other parts look fine. Almost any NPN transistor with sufficient Hfe should do the job. I'm using BC550C. The relay should be 12V one.

### Details

The cheap SIM800C I got was designed for 5V. There were two diodes (the one in red) that should lower the 5V below 4.2V (this is the highest acceptable voltage for SIM800C). Well, they didn't worked because even at 5.1V the voltage drop over them while the module was connecting to the network was so high, that the SIM800C VCC was getting below 3.3V and resetting. To resolve this - short circuit the diodes. This will achive higher efficiency. Both the Arduino (at 16MHZ) and the SIM800C operate totaly fine on 3.8V.

The diode between 3.8V and the Arduino 5V pin is to protect the SIM800C in case the arduino is connected to a computer USB port. According the SIM800C datasheet 5V VCC should fry it. The diode should be a low drop one.

The diode between SIM800C PWR pin and Arduino D4 is again for protecting the SIM800C. It allows the arduino to bring the PWR pin low and to switch on/off the SIM800C. I used a low drop diode, but I suspect that even regular diode here would be fine.

![GSM Remote wiring diagram](./schematic/gsm_remote.png "Wiring diagram")

## Programming the SIM800C

Several commands should be executed prior using the provided source code.
* ATE0&W -> Set ECHO to OFF and save the settings.
* AT+CLIP=1 -> Set CLIP mode to 1. This will provide the calling number on each RING.

## TODOs
* Add voltage monitoring circuit and code.
* Measure the power consumption over 24 hours and publish the results.