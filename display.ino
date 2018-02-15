/*
 Name:    Sketch4.ino
 Created: 1/28/2017 10:32:00 PM
 Author:  Sebastian
*/

#include "Tlc5940.h"
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>

#define rxPin 7
#define txPin 3
#define sirenPin 12

enum state { INIT, PLING, SCROLLING, SHOW_PLING };

//uint8_t plingCounter = 0;
uint8_t plingCounter;

state State = INIT;
SoftwareSerial mySerial(rxPin, txPin);

String incomingChars;
String scroll = "en ol   to ol   tre ol......  dass dass";

//Timer variables
unsigned long changeStateTimer;
unsigned long lastUpdateTimer;
unsigned long plingResetTimer;

//interrupt stuff
const uint8_t sensePin = 2;
//int sirenPin = 12;
bool hasTriggered = false;



void setup()
{
  pinMode(sensePin, INPUT_PULLUP);
  pinMode(sirenPin, OUTPUT);
  //digitalWrite(sirenPin, LOW);
  //digitalWrite(sirenPin, HIGH);

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  mySerial.begin(9600);
  /* Call Tlc.init() to setup the tlc.
  You can optionally pass an initial PWM value (0 - 4095) for all channels.*/
  Tlc.init();

  //INT0 setup
  //EEPROM.write(0, 0);


  Serial.println("Serial started\n");

  delay(5000);

  plingCounter = EEPROM.read(0);

  State = SCROLLING;
  
  changeStateTimer = millis();
  lastUpdateTimer = millis();
  //attachInterrupt(INT0, ISR0, FALLING);
  //sei();
}



void loop()
{
  
  
  if (digitalRead(sensePin) == LOW) {
    State = PLING;

  }
  
  //change state periodically
  
  if (changeStateTimer + 10000 <= millis()) {
    if (State == SHOW_PLING) {
      State = SCROLLING;
    }
    else if (State == SCROLLING) {
      State = SHOW_PLING;
      
      Tlc.clear();
      setSlice("pling   ");
      setNumFromRight(plingCounter, 0);
      Tlc.update();
    }

    changeStateTimer = millis();
  }
  
  
  //check if new string from network
  if (mySerial.available()) {
    processByte(mySerial.read());
  }


  switch (State) {
  case PLING:
    //Increment counter in interrupt routine and set plingResetTimier to millis()
    plingCounter++;
    plingResetTimer = millis();
    EEPROM.write(0, plingCounter);

    Tlc.clear();
    setSlice("trig    ");
    setNumFromRight(plingCounter, 0);
    Tlc.update();
    //Cut music and start siren
    digitalWrite(sirenPin, LOW);
    delay(4000);

    //start music and cut siren
    //hasTriggered = false;
    digitalWrite(sirenPin, HIGH);
    State = SCROLLING;
    changeStateTimer = millis();

  case SCROLLING:
    //scrolling
    if (lastUpdateTimer + 200 <= millis()) {
      Tlc.clear();
      scrollingDisplay(scroll);
      Tlc.update();

      lastUpdateTimer = millis();
    }
  }
}

void processByte(const byte inByte) {
  char c = inByte;

  switch (inByte) {
  case '\n':
    scroll = incomingChars;
    incomingChars = "";
  case '\r':
    break;
  default:
    incomingChars += c;
  }
}

// Font array
unsigned const char font[39] =
{
  0b01111110, // 0
  0b00110000, // 1
  0b01101101, // 2
  0b01111001, // 3
  0b00110011, // 4
  0b01011011, // 5
  0b01011111, // 6
  0b01110000, // 7
  0b01111111, // 8
  0b01111011, // 9
  0b01110111, // A
  0b00011111, // b
  0b01001110, // C
  0b00111101, // D
  0b01001111, // E
  0b01000111, // F
  0b01011110, // G
  0b00010111, // h
  0b00000110, // I
  0b00111000, // J
  0b00110111, // K
  0b00001110, // L
  0b01010100, // M
  0b00010101, // n
  0b00011101, // O
  0b01100111, // P
  0b01110011, // q
  0b00000101, // r
  0b01011011, // S
  0b00001111, // t
  0b00111110, // U
  0b00111110, // v
  0b00111111, // W
  0b00110111, // X
  0b00111011, // Y
  0b01101101, // Z
  0b00000001, // -
  0b00000000, // blank
  0b10000000, // decimal point
};

// sets a single char in the desired digit
void setDigit(char ch, int digit) {
  char fontCh;

  if (ch == ' ') {
    fontCh = font[37];
  }
  else if (ch == '.') {
    fontCh = font[38];
  }
  else if (ch >= 48 && ch <= 57) {
    fontCh = font[ch - 48];
  }
  else if (ch >= 65 && ch <= 90) {
    fontCh = font[ch - 55];
  }
  else if (ch >= 97 && ch <= 122) {
    fontCh = font[ch - 87];
  }
  else {
    return;
  }
  if (digit <= NUM_TLCS*2) {
    for (int channel = digit * 8; channel < digit * 8 + 8; channel++) {
      if (fontCh >> (7+(digit*8) - channel) & 1) {
        Tlc.set(channel, 4095);
      }
    }
  }
}

// sets an integer at the right side of the display
void setNumFromRight(int number, int start) {
  int mod = (number % 10);
  char ch = font[mod];
  for (int channel = start; channel < start+8; channel++) {
    if (ch >> (7+start - channel) & 1) {
      Tlc.set(channel, 4095);
    }
  }
  if (number >= 10) {
    setNumFromRight(number/10, (start + 8));
  }
}

// sets a string of size 8
void setSlice(String slice) {
  for (int i = 0; i < NUM_TLCS*2; i++) {
    setDigit(slice.charAt((NUM_TLCS*2-1)-i), i);
  }
}

// does one iteration of the scrolling display
// keeps count by static variable
// Could be used with a timer interrupt f.ex.
void scrollingDisplay(String str) {
  String temp = "          " + str + "          ";
  static uint8_t offset = 0;
  String slice = temp.substring(offset, offset + 8);
  offset++;

  if (offset >= (temp.length()-9)) {
    offset = 0;
  }
  setSlice(slice);
}
