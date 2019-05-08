//------------------------------------------------
//  Arduino UNO I2C Master Controller - by Adam Brackpool
//
//  I2C Master Controller is used to control the I2C 'traffic' between any number of 'I/O' Slave Arduinos on the network
//  Simple usage would be for more than 2 arduinos to pass variable information between each other - ie button presses and light LEDs
//  I2C protocol is comprised of 2 wires, SDA and SCL, and includes no Arbitrage of buss line ownership.
//  Therefore, this system employs a 3rd wire, denoted as triggerWire, which is connected to pin #6 on every arduino with a 10k pullup resistor to 5V
//  When a Slave device has new information to pass, it pulls this line low, triggering the Master Controller to poll each Slave successively.
//  The Master updates it bank of variables and then Writes these to all salves that require the information
//
//  Consideration may be made to allocate an individual, unique trigger pin on the master to each slave so it only polls the Slave with new information
//
//  Git Repo for project at:
//  https://github.com/adds666/CamperControl
//  All code is bastardised to some extent, credit is given in the git repo

//  Slave1 has an I2C address of 0x09 - can I get this in to the code?
//------------------------------------------------ - //
#include<Wire.h>

volatile bool heaterOn = false;
volatile bool lightsOn = false;

const int triggerWire = 6;   // '3rd' buss wire in the I2C buss system - Pulled high with 10k resistors and low by other arduinos to trigger 'Polling'

void setup()
{
  Serial.begin(9600);
  Wire.begin(0x08);     //a provision for the Slave to access Now Master as Slave if needed

  pinMode(triggerWire, INPUT);

}

void loop(){
  
  if (digitalRead(triggerWire == LOW)) {  // If the triggerWire is pulled LOW (by a Slave) instigate polling of data
    pollSlaves();
  }
}

void pollSlaves() {
  Wire.requestFrom(0x09, 2);  // request 2 bytes (number of Variables) from device address 0x09

  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }
}
