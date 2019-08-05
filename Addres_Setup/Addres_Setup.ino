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
