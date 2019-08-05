# CamperControl
Control system for Campervan 

Following on from a question on the Arduino StackExchange I developed a system for having a "rolling master".

The design objective is to have:


Multiple devices (eg. Arduino Unos or equivalent in a smaller form factor, such as a Nano or Pro Mini) in your house.
Each is connected to a "bus" of a pair of wires (preferably twisted-pair like is used for Ethernet cabling)
Each one has one or more sensors (eg. light sensor, switches, movement sensors) which it tests
Each one may also control things like lights, TV power, curtains opening, etc.
The system should be tolerant of any device going offline (ie, failing or being powered off)
Each device reports its status to the other devices, so that a switch on device A might turn on a light connected to device B, for example.


What I implemented is:



Each device has its own address, which it gets from EEPROM. eg. 0, 1, 2, 3, 4, 5 ...

You choose a range of addresses you are going to use (eg. maximum of 10)

When the device powers up it first listens for other devices "talking" on the bus. Hopefully it will hear at least one other (if not, see below).

We decide on a fixed "message packet", say of 50 bytes including address, CRC, etc. At 9600 baud that would take 52 ms to send.

Each device gets a "slot" of time, and waits its turn to talk to the bus.

When its timeslot arrives, it goes into output mode, and broadcasts its packet which includes its own address. Therefore all the other devices can now read its status (and act upon it if necessary). Eg. device 1 might report that switch 3 is closed, which means device 2 must turn a light on.

Ideally, you know your timeslot has arrived because your device address is one greater than the packet you just listened to. Eg. You are device 3. You just heard device 2 announce its status. Now it's your turn. Of course, you wrap around at the maximum number, so after device 9 you go back to device 0.

If a device is missing and does not respond, you give it a timeslot worth of time to respond, and then assume it is dead, and every device on the bus now assumes the next timeslot has started. (eg. You heard device 2, device 3 should respond, after 52 ms of inactivity, device 4 can now respond). This rule gives a device 52 ms to respond, which should be plenty even if it is servicing an interrupt or something like that.

If multiple devices in sequence are missing you count a 52 ms gap for each missing device, until it is your turn.

Once you get at least one response the timing is resynchronized, so any drift in clocks would be cancelled.



The only difficulty here is that upon initial power-up (which might happen simultaneously if the power to the building is lost and then restored) there is no device currently broadcasting its status, and thus nothing to synchronize to.

In that case:


If after listening long enough for all devices to be heard (eg. 500 ms) and hearing nothing, the device tentatively assumes it is the first one and makes a broadcast. However possibly two devices will do that at the same time, and thus never hear each other.

If a device has not heard from another device, it staggers the time between broadcasts randomly (seeding the random-number generator from its device number, to avoid all devices "randomly" staggering the broadcasts by the same amount).

This random staggering by additional amounts of time won't matter, because there is no-one listening anyway.

Sooner or later one device will get exclusive use of the bus and the other ones can then synchronize with it in the usual way.

This random gap between attempts to communicate is similar to what Ethernet used to do when multiple devices shared one coaxial cable.


Setting the device addresses


First you need to set up the current device address, and the number of devices, in EEPROM, so run this sketch, changing myAddress for each Arduino:



#include <EEPROM.h>

const byte myAddress = 3;
const byte numberOfDevices = 4;

void setup ()
  {
  if (EEPROM.read (0) != myAddress)
    EEPROM.write (0, myAddress);
  if (EEPROM.read (1) != numberOfDevices)
    EEPROM.write (1, numberOfDevices);

  }  // end of setup

void loop () { }


Code for rolling master


This code uses the RS485 library described earlier in this thread to ensure that a "packet" of data is received reliably.

It is up to you what data you broadcast in the "message" structure below. In the example there is the device address (needed so we know which device it is) followed by an array of 10 bytes (eg. these could be 10 switch positions) plus another "status" integer, which might be the result of reading from a light sensor.
