/*
 * EMON NOTIFY 
 * 
 * ARDUINO BASED DISPLAY TOOL FOR VISUAL READINGS OF OPEN ENERGY MONITOR EMONCMS FEEDS
 * 
 * NEEDS RFM RECEIVER, AND MAX7219 BASED 8x8 SEGMENT LED OUTPUT MODULE

MIT License
Copyright (c) 2017 Stuart Pittaway
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#define RF69_COMPAT 0                                                              // Set to 1 if using RFM69CW or 0 is using RFM12B

#define RF_freq RF12_433MHZ                                              // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.

#define MAXIMUM_READING_BUFFER 20
#define RF_FrequencyCorrection  1600

#include <JeeLib.h>
#include <TimeLib.h>

#include "LedControl.h"


//9 = emontx2, 10= emontx1, 5=emonpi
byte nodeID = 4;
const int networkGroup = 210;

LedControl lc1 = LedControl(7, 4, 3, 1);

float readings[MAXIMUM_READING_BUFFER];

//Time between pages in 10ms counts (500= 5 seconds)
#define TIME_BETWEEN_PAGES 250;

int page = 0;
int maxpage = 0;
int counter = TIME_BETWEEN_PAGES;

float Read4ByteFloat(uint8_t *d, int offset) {
  union ArrayToFloat {
    byte array[4];
    float value;
  };

  ArrayToFloat converter;

  converter.array[0] = d[offset + 0] ;
  converter.array[1] = d[offset + 1] ;
  converter.array[2] = d[offset + 2] ;
  converter.array[3] = d[offset + 3] ;

  return converter.value;
}

int32_t Read4ByteInt32(uint8_t *d, int offset) {
  union ArrayTouint32_t {
    byte array[4];
    int32_t integer;
  };

  ArrayTouint32_t converter;

  converter.array[0] = d[offset + 0] ;
  converter.array[1] = d[offset + 1] ;
  converter.array[2] = d[offset + 2] ;
  converter.array[3] = d[offset + 3] ;

  return converter.integer;
}

boolean RF_Rx_Handle() {

  if (rf12_recvDone()) {    //if RF Packet is received
    if (rf12_crc == 0) {    //Check packet is good

      //Only allow packets addressed to me
      if ((rf12_hdr & RF12_HDR_MASK) == nodeID    ) {
        return (1);
      }
    }
  }

  return (0);
}


void setup() {
  // put your setup code here, to run once:
  memset(readings, 0, sizeof(readings));

  Serial.begin(38400);

  Serial.println("OpenEnergyMonitor receiver");

  for (int index = 0; index < lc1.getDeviceCount(); index++) {
    lc1.shutdown(index, false);
    // Set the brightness to a medium value
    lc1.setIntensity(index, 2);
    // and clear the display
    lc1.clearDisplay(index);
  }

  rf12_initialize(nodeID, RF_freq, networkGroup, RF_FrequencyCorrection );                         // initialize RFM12B/rfm69CW

  //Sweep LED top and bottom lines just for fun
  for (int index = 0; index <= 7; index++) {
    lc1.setLed(0, index, 1, true);
    lc1.setLed(0, 7 - index, 4, true);
    delay(100);
    lc1.setLed(0, index, 1, false);
    lc1.setLed(0, 7 - index, 4, false);
  }

  lc1.clearDisplay(0);

  //test();
}

/*
void test() {
  outputReading(1234.56, 0); delay(2000);
  outputReading(1234.00, 0); delay(2000);
  outputReading(-5, 0); delay(2000);
  outputReading(-5.15, 0); delay(2000);
  outputReading(2.15, 0); delay(2000);
  outputReading(0.00, 0); delay(2000);
  outputReading(-0.15, 0); delay(2000);
  outputReading(-0.25, 0); delay(2000);
  outputReading(-0.55, 0); delay(2000);

  float f = -2.15;
  for (int index = 0; index < 500; index++) {
    //Serial.println(f);
    outputReading(f, 0);
    f += 0.03;
    delay(100);
  }
}
*/

void loop() {

  if (RF_Rx_Handle() == 1) {                                                    // Returns true if RF packet is received
    //We have just received a fresh packet of data from emonPI
    int i = 0;
    int x = 0;

    memset(readings, 0, sizeof(readings));
    while (i < rf12_len && x < MAXIMUM_READING_BUFFER ) {
      //First 4 bytes are the date time in EPOCH format

      if (x == 0) {
        //TIME
        setTime(Read4ByteInt32(rf12_data, i));
      } else {
        readings[x] = Read4ByteFloat(rf12_data, i);
      }

      //Serial.print(x); Serial.print(" value="); Serial.println(readings[x]);

      i += sizeof(int32_t);
      x++;
    }

    maxpage = x;
  }

  if (counter == 0) {
    counter = TIME_BETWEEN_PAGES;
    page++;

    if (page >= maxpage) {
      page = 0;
    }

    //Send to LCD display...
    lc1.clearDisplay(0);

    if (page == 0) {
      //Clock function
      int32_t m = (hour() * 100) + minute();
      outputReading(m, 2);
      //Enable the decimal point between hour and minute
      lc1.setLed(0, 4, 0, true);
    } else {
      outputReading(readings[page], 0);
      //Indicate which page this is on the left most digit
      lc1.setDigit(0, 7, (byte)page, false);
    }
  }

  //Short delay
  delay(10);
  counter--;
}

void outputReading(float value, int offset) {
  //Buffer...
  char s[10] = "          ";

  lc1.clearDisplay(0);

  float floored = floor(value);
  float remainder = value - floored;

  int decimaloffset = -1;

  if (remainder != 0)  {
    decimaloffset = ftoa(s, value);
  } else {
    itoa(floored, s, 10);
  }

  //Populate LED display from right position to left
  byte led = offset;
  for (int index = strlen(s) - 1; index >= 0; index--) {
    lc1.setChar(0, led, s[index], false);
    led++;
  }

  //Highlight the decimal place LED
  if (decimaloffset != -1) {
    lc1.setLed(0, strlen(s) - decimaloffset, 0, true);
  }


  //Serial.println(s);
}


int *ftoa(char *a, float f)
{
  //Copied from https://forum.arduino.cc/index.php?topic=59027.0
  //tweaked to force 2 DP and fix bug

  //long p[] = {0, 10, 100, 1000, 10000, 100000, 1000000};

  char *ret = a;
  long heiltal = (long)f;

  if (f > -1.00 && f < 0) {
    //Looks like this has a bug with small negative numbers less than one
    //for instance -0.25 appears as 0.25, as you cannot have "negative zero" in toe itoa routine, so fix that here
    *a++ = '-';
    *a++ = '0';
  } else {
    itoa(heiltal, a, 10);
    while (*a != '\0') a++;
  }

  //Keep track of where the decimal place is
  //*a++ = '.';
  int decimaloffset = a - ret;

  //long desimal = abs((long)((f - heiltal) * p[precision]));
  long desimal = abs((long)((f - heiltal) * 100));
  itoa(desimal, a, 10);

  return decimaloffset;
}
