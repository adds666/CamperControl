/*
 Multi-drop RS485 device control demo.

 Devised and written by Nick Gammon.
 Date: 7 September 2015
 Version: 1.0

 Licence: Released for public use.

 For RS485_non_blocking library see: http://www.gammon.com.au/forum/?id=11428
 For JKISS32 see: http://forum.arduino.cc/index.php?topic=263849.0
*/

#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// the data we broadcast to each other device
//struct
//  {
//  byte address;
//  byte switches [10];
//  int  status;
//  }  message;
  
struct
  {
  byte address;
  bool boolStates [4];
  }  message;

const unsigned long BAUD_RATE = 9600;
const float TIME_PER_BYTE = 1.0 / (BAUD_RATE / 10.0);  // seconds per sending one byte
const unsigned long PACKET_LENGTH = ((sizeof (message) * 2) + 6); // 2 bytes per payload byte plus STX/ETC/CRC
const unsigned long PACKET_TIME =  TIME_PER_BYTE * PACKET_LENGTH * 1000000;  // microseconds

// software serial pins
const byte RX_PIN = 2;
const byte TX_PIN = 3;
// transmit enable
const byte XMIT_ENABLE_PIN = 4;

// debugging pins
//const byte OK_PIN = 11;
//const byte TIMEOUT_PIN = 12;
//const byte SEND_PIN = 13;
//const byte SEARCHING_PIN = A5;
//const byte ERROR_PIN = A4;

// action pins (demo)
//const byte LED_PIN = 13;
//const byte SWITCH_PIN = A0;

// action pins (Push_button_PAnel_485-01) *******************************************************

// from Momentary_push_button.ino) *****************************************/
const uint32_t debounceTime = 5; // 20 mSec, enough for most switches
const uint8_t heaterButton = 6; // with N.O momentary pb switch to ground
const uint8_t lightsButton = 7; // with N.O momentary pb switch to ground
const uint8_t sleepButton = 8; // with N.O momentary pb switch to ground
const uint8_t emergencyButton = 9; // with N.O momentary pb switch to ground

const byte heaterIndicator = 10; // 
const byte lightsIndicator = 11; // 
const byte sleepIndicator = 12; // 
const byte emergencyIndicator = 13; // 

const bool heaterButtonSwitchOn = false;  // using INPUT_PULLUP
const bool heaterButtonSwitchOff = true;
const bool lightsButtonSwitchOn = false;
const bool lightsButtonSwitchOff = true;
const bool sleepButtonSwitchOn = false;
const bool sleepButtonSwitchOff = true;
const bool emergencyButtonSwitchOn = false;
const bool emergencyButtonSwitchOff = true;

bool lastHeaterButtonState = heaterButtonSwitchOn;
bool newHeaterButtonState = heaterButtonSwitchOff;
bool lastLightsButtonState = lightsButtonSwitchOn;
bool newLightsButtonState = lightsButtonSwitchOff;
bool lastSleepButtonState = sleepButtonSwitchOn;
bool newSleepButtonState = sleepButtonSwitchOff;
bool lastEmergencyButtonState = emergencyButtonSwitchOn;
bool newEmergencyButtonState = emergencyButtonSwitchOff;

// Global Variables used in RS485 comms
bool heaterState = false;
bool lightsState = false;
bool sleepState = false;
bool emergencyState = false;

bool boolStates[4] = {heaterState, lightsState, sleepState, emergencyState};

// *************************************************************************/

// times in microseconds
const unsigned long TIME_BETWEEN_MESSAGES = 3000;
unsigned long noMessagesTimeout;

byte nextAddress;
unsigned long lastMessageTime;
unsigned long lastCommsTime;
unsigned long randomTime;

SoftwareSerial rs485 (RX_PIN, TX_PIN);  // receive pin, transmit pin

// what state we are in
enum {
   STATE_NO_DEVICES,
   STATE_RECENT_RESPONSE,
   STATE_TIMED_OUT,
} state;

// callbacks for the non-blocking RS485 library
size_t fWrite (const byte what)
  {
  rs485.write (what);
  }

int fAvailable ()
  {
  return rs485.available ();
  }

int fRead ()
  {
  lastCommsTime = micros ();
  return rs485.read ();
  }

// RS485 library instance
RS485 myChannel (fRead, fAvailable, fWrite, 20);

// from EEPROM
byte myAddress;        // who we are
byte numberOfDevices;  // maximum devices on the bus

// Initial seed for JKISS32
static unsigned long x = 123456789,
                     y = 234567891,
                     z = 345678912,
                     w = 456789123,
                     c = 0;

// Simple Random Number Generator
unsigned long JKISS32 ()
  {
  long t;
  y ^= y << 5;
  y ^= y >> 7;
  y ^= y << 22;
  t = z + w + c;
  z = w;
  c = t < 0;
  w = t & 2147483647;
  x += 1411392427;
  return x + y + w;
  }  // end of JKISS32

void Seed_JKISS32 (const unsigned long newseed)
  {
  if (newseed != 0)
    {
    x = 123456789;
    y = newseed;
    z = 345678912;
    w = 456789123;
    c = 0;
    }
  }  // end of Seed_JKISS32

void setup ()
  {
  // debugging prints
  Serial.begin (115200);
  // software serial for talking to other devices
  rs485.begin (BAUD_RATE);
  // initialize the RS485 library
  myChannel.begin ();

  // debugging prints
  Serial.println ();
  Serial.println (F("Commencing"));
  myAddress = EEPROM.read (0);
  Serial.print (F("My address is "));
  Serial.println (int (myAddress));
  numberOfDevices = EEPROM.read (1);
  Serial.print (F("Max address is "));
  Serial.println (int (numberOfDevices));

  if (myAddress >= numberOfDevices)
    Serial.print (F("** WARNING ** - device number is out of range, will not be detected."));

  Serial.print (F("Packet length = "));
  Serial.print (PACKET_LENGTH);
  Serial.println (F(" bytes."));

  Serial.print (F("Packet time = "));
  Serial.print (PACKET_TIME);
  Serial.println (F(" microseconds."));

  // calculate how long to assume nothing is responding
  noMessagesTimeout = (PACKET_TIME + TIME_BETWEEN_MESSAGES) * numberOfDevices * 2;

  Serial.print (F("Timeout for no messages = "));
  Serial.print (noMessagesTimeout);
  Serial.println (F(" microseconds."));

  // set up various pins
  pinMode (XMIT_ENABLE_PIN, OUTPUT);

  // demo action pins
//  pinMode (SWITCH_PIN, INPUT_PULLUP);
//  pinMode (LED_PIN, OUTPUT);

// from Momentary_push_button.ino) *****************************************/
  pinMode (heaterButton, INPUT_PULLUP);
  pinMode (lightsButton, INPUT_PULLUP);
  pinMode (sleepButton, INPUT_PULLUP);
  pinMode (emergencyButton, INPUT_PULLUP);


// *************************************************************************/
  // debugging pins
//  pinMode (OK_PIN, OUTPUT);
//  pinMode (TIMEOUT_PIN, OUTPUT);
//  pinMode (SEND_PIN, OUTPUT);
//  pinMode (SEARCHING_PIN, OUTPUT);
//  pinMode (ERROR_PIN, OUTPUT);

  pinMode (heaterIndicator, OUTPUT);
  pinMode (lightsIndicator, OUTPUT);
  pinMode (sleepIndicator, OUTPUT);
  pinMode (emergencyIndicator, OUTPUT);

  // seed the PRNG
  Seed_JKISS32 (myAddress + 1000);

  state = STATE_NO_DEVICES;
  nextAddress = 0;

  randomTime = JKISS32 () % 500000;  // microseconds
  Serial.println ("End of Setup");

  }  // end of setup

// set the next expected address, wrap around at the maximum
void setNextAddress (const byte current)
  {
  nextAddress = current;
  if (nextAddress >= numberOfDevices)
    nextAddress = 0;
  }  // end of setNextAddress


// Here to process an incoming message
void processMessage ()
  {
Serial.println("incoming message");
  // we cannot receive a message from ourself
  // someone must have given two devices the same address
  if (message.address == myAddress)
    {
    //digitalWrite (ERROR_PIN, HIGH);
    while (true)
      { }  // give up
    }  // can't receive our address

  //digitalWrite (OK_PIN, HIGH);

  // handle the incoming message, depending on who it is from and the data in it

  // make our LED match the switch of the previous device in sequence
//  if (message.address == (myAddress - 1));
//    digitalWrite (LED_PIN, message.switches [0]);
//
//  digitalWrite (OK_PIN, LOW);
  Serial.print("Heater State before is");
  Serial.println(heaterState);
  if (message.address == (1));
      heaterState = (message.boolStates [0]);
      lightsState = (message.boolStates [1]);
      sleepState = (message.boolStates [2]);
      emergencyState = (message.boolStates[3]);

  Serial.print("Heater State after is");
  Serial.println(heaterState);  
  //digitalWrite (OK_PIN, LOW);
  } // end of processMessage

// Here to send our own message
void sendMessage ()
  {
    Serial.println ("SendMessage begin");
  //digitalWrite (SEND_PIN, HIGH);
  memset (&message, 0, sizeof message);
  message.address = myAddress;

  // fill in other stuff here (eg. switch positions, analog reads, etc.)

  // message.switches [0] = digitalRead (SWITCH_PIN);

  message.boolStates [0] = heaterState;
  message.boolStates [1] = lightsState;
  message.boolStates [2] = sleepState;
  message.boolStates [3] = emergencyState;
  
  // now send it
  digitalWrite (XMIT_ENABLE_PIN, HIGH);  // enable sending
  myChannel.sendMsg ((byte *) &message, sizeof message);
  digitalWrite (XMIT_ENABLE_PIN, LOW);  // disable sending
  setNextAddress (myAddress + 1);
//  digitalWrite (SEND_PIN, LOW);

  lastCommsTime = micros ();   // we count our own send as activity
  randomTime = JKISS32 () % 500000;  // microseconds
  }  // end of sendMessage

void loop ()
  {

     heaterButtonCheck();
     sleepButtonCheck();
     lightButtonCheck();
     emergencyButtonCheck();
     indicatorCheck();
    
  // incoming message?
  if (myChannel.update ())
    {
    memset (&message, 0, sizeof message);
    int len = myChannel.getLength ();
    if (len > sizeof message)
      len = sizeof message;
    memcpy (&message, myChannel.getData (), len);
    lastMessageTime = micros ();
    setNextAddress (message.address + 1);
    processMessage ();
    state = STATE_RECENT_RESPONSE;
    }  // end of message completely received

  // switch states if too long a gap between messages
  if  (micros () - lastMessageTime > noMessagesTimeout)
    state = STATE_NO_DEVICES;
  else if  (micros () - lastCommsTime > PACKET_TIME)
    state = STATE_TIMED_OUT;

  switch (state)
    {
    // nothing heard for a long time? We'll take over then
    case STATE_NO_DEVICES:
      if (micros () - lastCommsTime >= (noMessagesTimeout + randomTime))
        {
        Serial.println (F("No devices."));
//        digitalWrite (SEARCHING_PIN, HIGH);
        sendMessage ();
//        digitalWrite (SEARCHING_PIN, LOW);
        }
      break;

    // we heard from another device recently
    // if it is our turn, respond
    case STATE_RECENT_RESPONSE:
      // we allow a small gap, and if it is our turn, we send our message
      if (micros () - lastCommsTime >= TIME_BETWEEN_MESSAGES && myAddress == nextAddress)
        sendMessage ();
      break;

    // a device did not respond in its slot time, move onto the next one
    case STATE_TIMED_OUT:
//      digitalWrite (TIMEOUT_PIN, HIGH);
      setNextAddress (nextAddress + 1);
      lastCommsTime += PACKET_TIME;
//      digitalWrite (TIMEOUT_PIN, LOW);
      state = STATE_RECENT_RESPONSE;  // pretend we got the missing response
      break;

    }  // end of switch on state

  }  // end of loop

    
 void heaterButtonCheck(){   
    // from Momentary_push_button.ino) *****************************************/
    // Check whether heaterButton has been pressed and change the Global Variable HeaterState if it has.
    Serial.println ("Commence heaterButtonCheck");
    newHeaterButtonState = digitalRead(heaterButton);

    if(lastHeaterButtonState != newHeaterButtonState) // state changed
    {
      delay(debounceTime);
      lastHeaterButtonState = newHeaterButtonState;

      // push on, push off
      if(newHeaterButtonState == heaterButtonSwitchOn && heaterState == false)
      {
        heaterState = true;
        Serial.println(F("Switched Heater ON"));
      }
      else if(newHeaterButtonState == heaterButtonSwitchOn && heaterState == true)
      {
        heaterState = false;
        Serial.println(F("Heater Switched OFF"));
      }
    }
    Serial.println("heaterButtonCheck complete");
 } // End of heaterButtonCheck();

void sleepButtonCheck(){
    // Check whether heaterButton has been pressed and change the Global Variable HeaterState if it has.
    newSleepButtonState = digitalRead(sleepButton);

    if(lastSleepButtonState != newSleepButtonState) // state changed
    {
      delay(debounceTime);
      lastSleepButtonState = newSleepButtonState;

      // push on, push off
      if(newSleepButtonState == sleepButtonSwitchOn && sleepState == false)
      {
        sleepState = true;
        Serial.println(F("Switched sleep state ON"));
      }
      else if(newSleepButtonState == sleepButtonSwitchOn && sleepState == true)
      {
        sleepState = false;
        Serial.println(F("Sleep state Switched OFF"));
      }
    }
} // End of sleepButtonCheck();

void lightButtonCheck(){
    // Check whether lightsButton has been pressed and change the Global Variable lightsState if it has.
    newLightsButtonState = digitalRead(lightsButton);

    if(lastLightsButtonState != newLightsButtonState) // state changed
    {
      delay(debounceTime);
      lastLightsButtonState = newLightsButtonState;

      // push on, push off
      if(newLightsButtonState == lightsButtonSwitchOn && lightsState == false)
      {
        lightsState = true;
        Serial.println(F("Switched Lights ON"));
      }
      else if(newLightsButtonState == lightsButtonSwitchOn && lightsState == true)
      {
        lightsState = false;
        Serial.println(F("Lights Switched OFF"));
      }
    }
} // End of lightButtonCheck();

void emergencyButtonCheck(){
    // Check whether lightsButton has been pressed and change the Global Variable lightsState if it has.
    newEmergencyButtonState = digitalRead(emergencyButton);

    if(lastEmergencyButtonState != newEmergencyButtonState) // state changed
    {
      delay(debounceTime);
      lastEmergencyButtonState = newEmergencyButtonState;

      // push on, push off
      if(newEmergencyButtonState == emergencyButtonSwitchOn && emergencyState == false)
      {
        emergencyState = true;
        Serial.println(F("Emergency State ON"));
      }
      else if(newEmergencyButtonState == emergencyButtonSwitchOn && emergencyState == true)
      {
        emergencyState = false;
        Serial.println(F("Emergency state OFF"));
      }
    }
} // End of emergencyButtonCheck();

void indicatorCheck(){
  Serial.println("Updating Indicators");
    if(heaterState == false)
    {
      digitalWrite (heaterIndicator, LOW);
    }
    else if(heaterState == true)
    {
      digitalWrite (heaterIndicator, HIGH);
    }

    if(lightsState == false)
    {
      digitalWrite (lightsIndicator, LOW);
    }
    else if(lightsState == true)
    {
      digitalWrite (lightsIndicator, HIGH);
    }

    if(sleepState == false)
    {
      digitalWrite (sleepIndicator, LOW);
    }
    else if(sleepState == true)
    {
      digitalWrite (sleepIndicator, HIGH);
    }

    if(emergencyState == false)
    {
      digitalWrite (emergencyIndicator, LOW);
    }
    else if(emergencyState == true)
    {
      digitalWrite (emergencyIndicator, HIGH);
    }
    Serial.println("Indicators are up to date");
} // End of indicatorCheck();
