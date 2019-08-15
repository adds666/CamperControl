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

// From RTC_Light_alarm *********************/
// and https://www.dummies.com/computers/arduino/how-to-display-the-time-for-your-arduino-clock-project/

#include <RTClib.h> // RTC Library
#include <Wire.h> // I2C Library

RTC_DS1307 rtc;
DateTime now;

// the data we broadcast to each other device
struct
  {
  byte address;
  int timeData [3];
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
//const byte OK_PIN = 6;
//const byte TIMEOUT_PIN = 7;
//const byte SEND_PIN = 8;
//const byte SEARCHING_PIN = 9;
//const byte ERROR_PIN = 10;

// action pins (demo)
//const byte LED_PIN = 13;
//const byte SWITCH_PIN = A0;

const byte heaterIndicator = 10; // 
const byte lightsIndicator = 11; // 
const byte sleepIndicator = 12; // 
const byte emergencyIndicator = 13; // 

// Global Variables used in RS485 comms
bool heaterState = false;
bool lightsState = false;
bool sleepState = false;
bool emergencyState = false;

int timeHour; // Setup an integer called timeHour
int timeMinute; // Setup an integer called timeMinute
int timeSecond; // Setup an integer called timeSecond

int timeData[3] = {timeHour, timeMinute, timeSecond};

// ******************************************/

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

// From RTC_Light_Alarm **************************/
// and https://www.dummies.com/computers/arduino/how-to-display-the-time-for-your-arduino-clock-project/

  if(!rtc.begin())
  {
    Serial.println("Couldn't find RTC"); // Error Message!
    while (1);  
  }
  if (!rtc.isrunning())
  {
    Serial.println("RTC is NOT running!"); // Error Message!
   // Sync RTC with the system clock at the time of compilation
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Uncomment – Compile – Upload (See Note)
  }
// **************************************************/
  
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
  
  if (message.address == (01));
    //digitalWrite (LED_PIN, message.switches [0]);
    
    heaterState == (message.boolStates [0]);
    lightsState == (message.boolStates [1]);
    sleepState == (message.boolStates [2]);
    emergencyState == (message.boolStates [3]);
    
  //digitalWrite (OK_PIN, LOW);
  } // end of processMessage

// Here to send our own message
void sendMessage ()
  {
  //digitalWrite (SEND_PIN, HIGH);
  memset (&message, 0, sizeof message);
  message.address = myAddress;

  // fill in other stuff here (eg. switch positions, analog reads, etc.)

  // message.switches [0] = digitalRead (SWITCH_PIN);

  message.timeData [0] = timeHour;
  message.timeData [1] = timeMinute;
  message.timeData [2] = timeSecond;

  // now send it
  digitalWrite (XMIT_ENABLE_PIN, HIGH);  // enable sending
  myChannel.sendMsg ((byte *) &message, sizeof message);
  digitalWrite (XMIT_ENABLE_PIN, LOW);  // disable sending
  setNextAddress (myAddress + 1);
  //digitalWrite (SEND_PIN, LOW);

  lastCommsTime = micros ();   // we count our own send as activity
  randomTime = JKISS32 () % 500000;  // microseconds
  }  // end of sendMessage

void loop ()
  {
    
  updateTime();
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
        //digitalWrite (SEARCHING_PIN, HIGH);
        sendMessage ();
        //digitalWrite (SEARCHING_PIN, LOW);
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
      //digitalWrite (TIMEOUT_PIN, HIGH);
      setNextAddress (nextAddress + 1);
      lastCommsTime += PACKET_TIME;
      //digitalWrite (TIMEOUT_PIN, LOW);
      state = STATE_RECENT_RESPONSE;  // pretend we got the missing response
      break;

    }  // end of switch on state

  }  // end of loop

void updateTime(){
    // From RTC_Light_Alarm **************************/
    // and https://www.dummies.com/computers/arduino/how-to-display-the-time-for-your-arduino-clock-project/
    now = rtc.now(); // Get the current time

      timeHour = now.hour(); // Get the hours right now and store them in an integer called h
      timeMinute = now.minute(); // Get the minutes right now and store them in an integer called m
      timeSecond = now.second(); // Get the seconds right now and store them in an integer called s
  } // End of updateTime();


void indicatorCheck(){
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
} // End of indicatorCheck();
