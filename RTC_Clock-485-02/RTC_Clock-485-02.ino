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

#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

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
  int timeStates [3];
  int dateStates [3];
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

// Global Variables used in RS485 comms
int timeHour;
int timeMinute;
int timeSecond;

int dateDay;
int dateMonth;
int dateYear; 

int timeStates[3] = {timeHour, timeMinute, timeSecond};
int dateStates[3] = {dateDay, dateMonth, dateYear};

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

// *************************************************************************/
  // debugging pins
//  pinMode (OK_PIN, OUTPUT);
//  pinMode (TIMEOUT_PIN, OUTPUT);
//  pinMode (SEND_PIN, OUTPUT);
//  pinMode (SEARCHING_PIN, OUTPUT);
//  pinMode (ERROR_PIN, OUTPUT);

  // seed the PRNG
  Seed_JKISS32 (myAddress + 1000);

  state = STATE_NO_DEVICES;
  nextAddress = 0;

  randomTime = JKISS32 () % 500000;  // microseconds
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
//  if (message.address == (myAddress - 1));
//    digitalWrite (LED_PIN, message.switches [0]);
//
//  digitalWrite (OK_PIN, LOW);
    
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

  message.timeStates [0] = timeHour;
  message.timeStates [1] = timeMinute;
  message.timeStates [2] = timeSecond;
  
  
  // now send it
  digitalWrite (XMIT_ENABLE_PIN, HIGH);  // enable sending
  myChannel.sendMsg ((byte *) &message, sizeof message);
  digitalWrite (XMIT_ENABLE_PIN, LOW);  // disable sending
  setNextAddress (myAddress + 1);
//  digitalWrite (SEND_PIN, LOW);

  lastCommsTime = micros ();   // we count our own send as activity
  randomTime = JKISS32 () % 500000;  // microseconds
  }  // end of sendMessage

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

void loop ()
  {
    tmElements_t tm;

  if (RTC.read(tm)) {

// Very eperimental - trying to get time out to int variables ready for
// distribution via RS485
    
    timeHour = (print2digits(tm.Hour));
    timeMinute = print2digits(tm.Minute);
    timeSecond == print2digits(tm.Second);
    dateDay == (tm.Day);
    dateMonth == (tm.Month);
    dateYear == (tmYearToCalendar(tm.Year));
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    

     
    
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
