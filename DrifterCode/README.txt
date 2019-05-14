1. To Run Drifter Nodes

If the drifter is new you must assign it a node number.
This must be 3 decimal digits between 103 and 254. This
will be stored in EEPROM on the chip along with the size
of the SPIFlash (which must be 4, 64 or 128 Mbits). Use
the code "node_initialize".

To erase the SPIFlash and start fresh use SPIFlash_ReadWrite.
Our's is a modified version of LowPowerLabs version. Use ours.
It will dump or erase the SPIFlash.

The drifter will start storing GPS data at the first blank
address in the SPIFlash. If you dump the data via any of our
options it will start at address = 0 and continue till it
finds a blank address.

Load code "Node_v2". This only needs to be done once.

It will start with sampling of GPS stopped, regardless
of whether it is a fresh load or a reset.

You must get it going by inputting a "g" either on the
serial monitor or on the RF Controller. If you start it from 
the serial monitor it will be in debug mode and will send 
lots of information to the screen.

You can start GPS sampling with "g" or stop it with "s". If you
want to dump the flash (with "d") it is best to stop it first.

If you start it from the RF controller it will not be in debug
mode and will not try to send anything to the serial monitor
even if the serial monitor is connected. It will not send the
debug messages to the RF controller either but it will allow you
to dump the data to the RF controller.

NB. The node latitude and longitude are in decimal degrees and
both are positive. Ideally this should be sorted out so they
automatically work anywhere in the world. This will require
figuring out how the GPS chip handles longitude. The latitude
should be easy to fix.

To Run the RF controller load "dump_flash_gateway". This will run
on any Moteino Mega and does not require GPS or SD card, just a 
bare board is sufficient. However you will need the serial monitor
to control it. It will tell you who it is and ask you to enter the
node number you wish to control. This must be 3 decimal digits
between 103 and 254. To start the GPS enter "g" for GO. Enter "s" 
for stop and "d" for dump SPIFlash. If you want to start all the
nodes at the same time type "G" to broadcast GO to all nodes.
Likewise typing "S" will broadcast STOP to all nodes.

2. To make a new drifter node you need to solder it to a green board.
If it is just the drifter it needs only a GPS module. If it is a gateway
it will also need an SD card. If an SD card will be used the board will
have to be modified to add a wire from the SD card connector the the
Moteino. This must pass through a hole in the board. See an existing
boards for exactly what must be done. All boards (except the RF controller)
need two bright LEDs. However the board must be modified so that these
can be made "extra-bright". This requires reversing the polarity of the
LED drive, so the LEDs conduct from +5 to Ground through the Moteino
instead of the present system in which they conduct from the Moteino to
ground. Again, look carefully at an existing board to see the simplest
modification.

With the modified LED drive the current increases from 20ma to 40ma on
each LED. This provides enough load to keep the power supplies from
turning themselves off. I have not tested to find the minimum "blink"
duration that will work. At present they blink ON twice for 1000ms each
time during the 10 sec GPS cycle.

At present one board is setup with sockets so chips and be installed, tested
and removed. That should be enough because the Moteinos can be tested
without the other chips.

We should be able to make a regular Moteino work as an RF controller but it
requires code modifications which I have not (yet) been able to get working.
Its probably better to stick to MEGAs so the code is completely interchangeable.

Ideally it would be nice to drive the RF Controller from a python program on a laptop. This has not been tested (yet).

