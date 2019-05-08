# CamperControl
Control system for Campervan - can be used in any scenario

Looking to create a network of Arduino's that each include inputs and outputs. Functionality of the system being that any input on any arduino can action an output on any other arduino.

This post is to outline my first thoughts and garner your opinions.

System comprises of one Master which will handle the I2C traffic, polling slaves, re-writing variables.
(forgot to include trigger line but simply one more buss line so slaves can tell master something has changed)

https://i.imgur.com/izAKOZw.jpg

Basic flow to be:

1. Slave button pressed (momentary)
2. Local State Change Detection and change local binary variable 
3. Slave pulls Trigger line LOW momentarily
4. Master polls Variables from all slaves in succession MasterReader
5. Master compares results to find what has changed
6. Master State change detection to change binary output variable (based on what has changed)
7. Master writes output variables to all slaves in succession MasterWriter
8. Slaves receive and change their Output variables and process action locally.


Using this approach I can break the programming (not my game) down in to manageable chunks.
