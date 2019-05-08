//------------------------------------------------
//  Arduino UNO I2C_Slave1 - by Adam Brackpool
//
//  I2C Slave1 is a slave device in an I2C network
//  It monitors the button presses of two momentary close buttons and if pressed
//  Changes the state of a variable which should then be passed to the master
//  https://www.arduino.cc/en/Tutorial/StateChangeDetection
//  In order to tell the master that it has new information, It pulls pin #6 LOW and triggers
//  A 'polling' function in the master to retrieve the variables
//
//  Git Repo for project at:
//  https://github.com/adds666/CamperControl
//  All code is bastardised to some extent, credit is given in the git repo

//  Master has an I2C address of 0x08 - can I get this in to the code?
//  Slave1 has an I2C address of 0x09 - can I get this in to the code?
//------------------------------------------------ - //
#include <Wire.h>

//volatile bool heaterOn = false; // these have changed to int's below to make easier for Wire.write to handle
//volatile bool lightsOn = false;

int heaterOn = 0; 
int lightsOn = 0;

const int triggerWire = 6;   // '3rd' buss wire in the I2C buss system - Pulled high with 10k resistors and low by other arduinos to trigger 'Polling'

const int heaterButton = 7; // momentary switch on 7, other side connected to ground
const int lightsButton = 8;  // momentary button on 8, otherside connected to ground

int heaterButtonState = 0;  // current state of the heaterButton
int lastHeaterButtonState = 0;  // previous state of the heaterButton

int lightsButtonState = 0; // current state of the lightsButton
int lastLightsButtonState = 0; // previous state of the lightsButton

unsigned long previousMillis = 0;        // will store last time triggerWire changed
const long interval = 200;           // interval at which to pull triggerWire LOW - may need tweaking?

void setup() {
  Wire.begin(0x09);             // join i2c bus with address 0x09
  Wire.onRequest(requestEvent); // register event

  pinMode(heaterButton, INPUT);
  pinMode(lightsButton, INPUT);
}

void loop() { // Look at all the buttons - act if one is pressed https://www.arduino.cc/en/Tutorial/StateChangeDetection
  heaterButtonState = digitalRead(heaterButton);  // Read the pushbutton input pin
  if (heaterButtonState != lastHeaterButtonState) { // compare the heaterButtonState to its previous state
    if (heaterButtonState == HIGH)  { // If high then the button went from Off to On
      if (heaterOn = 0) {
        heaterOn == 1;// Toggle Boolean variable
      } else if (heaterOn = 1)  {
        heaterOn == 0;
      }
      pollRequest(); // Request the Master retrieves variables via the triggerWire
    }
  }
  delay(20); // for debounce on the button
  lastHeaterButtonState = heaterButtonState;

  lightsButtonState = digitalRead(lightsButton);  // Read the pushbutton input pin
  if (lightsButtonState != lastLightsButtonState) { // compare the lightsButtonState to its previous state
    if (lightsButtonState == HIGH)  { // If high then the button went from Off to On
      if (lightsOn = 0) {
        lightsOn == 1;
      } else if (lightsOn = 1)  {
        lightsOn == 0;
      }
      pollRequest(); // Request the Master retrieves variables via the triggerWire
    }
  }
  delay(20); // for debounce on the button
  lastLightsButtonState = lightsButtonState;
}

void pollRequest() {
  digitalWrite(triggerWire, LOW);
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) { //
    previousMillis = currentMillis;

    digitalWrite(triggerWire, HIGH);
  }
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write(heaterOn);
  Wire.write(lightsOn);// does this send the two variable values?
}
