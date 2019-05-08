# CamperControl
Control system for Campervan - can be used in any scenario

Looking to create a network of Arduino's that each include inputs and outputs. Functionality of the system being that any input on any arduino can action an output on any other arduino.

This post is to outline my first thoughts and garner your opinions.

System comprises of one Master which will handle the I2C traffic, polling slaves, re-writing variables.
(forgot to include trigger line but simply one more buss line so slaves can tell master something has changed)

https://i.imgur.com/izAKOZw.jpg

Basic flow to be:

1. Slave button pressed (momentary)
2. Local State Change Detection and change local binary variable https://www.arduino.cc/en/Tutorial/StateChangeDetection
3. Slave pulls Trigger line LOW momentarily
4. Master polls Variables from all slaves in succession MasterReader https://www.arduino.cc/en/Tutorial/MasterReader
5. Master compares results to find what has changed
6. Master State change detection to change binary output variable (based on what has changed)
7. Master writes output variables to all slaves in succession MasterWriter https://www.arduino.cc/en/Tutorial/MasterWriter
8. Slaves receive and change their Output variables and process action locally.


Using this approach I can break the programming (not my game) down in to manageable chunks.


Assistance asked on Arduino forums and responses here:
https://forum.arduino.cc/index.php?topic=614704.0

Informative article
https://rheingoldheavy.com/arduino-wire-library/

I2C Devices within system - 
(Addressing checked against https://learn.adafruit.com/i2c-addresses/the-list as may want to include 'official' manufacturer made I2C sensors)

(Current State)

I2C_Master - 0x08 - To control and pass variables between input and output devices
I2C_Slave01 - 0x09 - Button Panel 1 - Momentary pushbutton input interface and LED light status output

(Future State)
I2C_Slave02 - 0x0A - RFID Entry
I2C_Slave03 - 0x0B - Alarm
I2C_Slave04 - 0x0C - SMS Interface
I2C_Slave05 - 0x0D - Radio IO (DTMF)
I2C_Slave06 - 0x0E - Power Control
I2C_Slave07 - 0x0F - Stereo Audio
I2C_Slave08 - 0x12 - Light Control
I2C_Slave09 - 0x14 - Button Panel 2

Global Variables - 

heaterOn - bool
lightsOn - bool
alarmArmed - bool
alarmTriggered - bool
personEntered - char / string
