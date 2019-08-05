/*
 * Arduino Wake-Up/Sunrise Light v1.0
 * Key Hardware: Arduino Uno/Nano & TinyRTC DS1307 Module
 * Arduino IDE: 1.6.9
 * Based on RTClib Library & Wire Library
 * Authored by T.K.Hareendran/2019
 * Published by www.electroschematics.com
 */

#include <RTClib.h>    // RTC Library
#include <Wire.h>      // I2C Library
//#define LEDPIN 3       // LED Drive O/P =D3
#define SWITCHPIN 2    // LED Switch I/P =D2
#define HOUR 16         // Wake-Up Time (hour)
#define MINUTE 19     // Wake-Up Time (minutes) //  i.e. Wake up at 5:40 (24hr clock)
#define FADESPEED 1000 // LED Fade Speed
#include <FastLED.h>
#define NUM_LEDS 60
#define DATA_PIN 2
CRGB leds[NUM_LEDS];

RTC_DS1307 rtc;
bool wake_up_now = false;

void setup()
{
  //pinMode(LEDPIN, OUTPUT);       // D3 as O/P
  pinMode(SWITCHPIN, INPUT);     // D2 as I/P
  digitalWrite(SWITCHPIN, HIGH); // Enable Internal Pull-Up Resistor
  Serial.begin(57600);           // Start Serial Monitor
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  if (!rtc.begin())
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
}

void loop()
{
  DateTime now = rtc.now();
  Serial.print(now.hour(), DEC); // Only For Serial Monitor
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

  //if (digitalRead(SWITCHPIN) == HIGH)
  //{
    // Wake-Up Configuration
    // Turn off the wake up light if LED switch is in OFF position
    /////////////////////////////****************************************************
    //analogWrite(LEDPIN, 0);
    //FastLED.clear ();
  //}
  if ((now.minute() == MINUTE) & (now.hour() == HOUR))
  {
    // Wake-Up Now
    wake_up_now = true;
  }
  else
  {
    // Pause for one second
    delay(1000);
  }
  // Turn on the wake up light if the LED switch is in ON position
  if (wake_up_now == true & digitalRead(SWITCHPIN) == HIGH)
  {
    for (int i = 0; i < 256; i++)
    {
      //analogWrite(LEDPIN, i);
      fill_solid(leds, NUM_LEDS, CHSV(24,100,i));
      FastLED.show();
      Serial.print(i);
      Serial.println();
      delay(FADESPEED);
      if (i == 254)
      {
        wake_up_now = false;
      }
    }
  }
}
