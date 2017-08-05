/*
   EMON NOTIFY

   ARDUINO BASED DISPLAY TOOL FOR VISUAL READINGS OF OPEN ENERGY MONITOR EMONCMS FEEDS

   NEEDS RFM RECEIVER, AND MAX7219 BASED 8x8 SEGMENT LED OUTPUT MODULE

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


#define RF69_COMPAT 1                                                              // Set to 1 if using RFM69CW or 0 is using RFM12B

#define RF_freq RF12_433MHZ                                              // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.

//Time between pages in 10ms counts (500= 5 seconds)
#define TIME_BETWEEN_PAGES 500;

//Time between reading one-wire temperature (30 seconds)
#define TIME_BETWEEN_TEMP_READINGS 3000;

#define MAXIMUM_READING_BUFFER 20
#define RF_FrequencyCorrection  1600

#define BACKLIGHT_LED_PIN 9
#define LCD_WIDTH 84
#define LCD_HEIGHT 48

#include <JeeLib.h>
#include <TimeLib.h>
#include <SPI.h>
#include "U8g2lib.h"

#include <OneWire.h>
#include <DallasTemperature.h>


#define ONE_WIRE_BUS 4                                                  // Data wire is plugged into port 2 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);                                          // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);                                    // Pass our oneWire reference to Dallas Temperature.

DeviceAddress address_T1 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //The DS address is not needed, on startup it will detect the first sensor

U8G2_PCD8544_84X48_F_4W_HW_SPI  u8g2(U8G2_R0, 7, 5, 6);  // HARDWARE SPI BUS PLUS CE=7, DC=5, RST=6

//9 = emontx2, 10= emontx1, 5=emonpi
byte nodeID = 4;
const int networkGroup = 210;

int tempSensors = 0;

float readings[MAXIMUM_READING_BUFFER];

int page = 1;
int maxpage = 0;

int temp_counter = 0;

int counter = 0;

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

  //Seed clock to a sensible date before we receive a reading from emonHUB
  setTime(1501545600);

  //Serial.begin(38400);  Serial.println("emonNOTIFY");

  // Set Backlight Intensity
  // 0 is Fully bright
  // 255 is off
  analogWrite(BACKLIGHT_LED_PIN, 0);

  u8g2.begin();

  sensors.begin();
  tempSensors = sensors.getDeviceCount();
  if (tempSensors != 0){
    sensors.getAddress(address_T1, 0);                  //get the address of the first detected sensor
  }    

  //initialize RFM12B/rfm69CW
  rf12_initialize(nodeID, RF_freq, networkGroup, RF_FrequencyCorrection );

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

//Display buffer
char s[20];
char header[20];

float temperature;

void loop() {

  if (tempSensors != 0 && temp_counter==0){
    sensors.requestTemperatures();                    // Request Temperature from sensors
    temperature = sensors.getTempC(address_T1);// * 100;

    temp_counter=TIME_BETWEEN_TEMP_READINGS;
  }
  
  if (RF_Rx_Handle() == 1) {                                                    
    // Returns true if RF packet is received so we have just received a fresh packet of data from emonPI
    int i = 0;
    int x = 0;

    memset(readings, 0, sizeof(readings));
    while (i < rf12_len && x < MAXIMUM_READING_BUFFER ) {
      //First 4 bytes are the date time in EPOCH format

      //TODO: MORE PACKET HANDLING HERE

      if (x == 0) {
        //TIME
        setTime(Read4ByteInt32(rf12_data, i));
      } else {
        readings[x] = Read4ByteFloat(rf12_data, i);
        //Serial.print(x); Serial.print(" value="); Serial.println(readings[x]);
      }

      i += sizeof(int32_t);
      x++;
    }

    maxpage = x;
  }

  if (counter == 0) {
    counter = TIME_BETWEEN_PAGES;
    page++;

    if (page >= maxpage) {
      page = 1;
    }

    if (maxpage == 0) {
      //We dont have any data yet

    } else {
      //Clear buffers
      memset(s, 0, sizeof(s));
      outputReading(s, readings[page], 0);

      //LCD is 84x48 pixels
      u8g2.clearBuffer();
      u8g2.setDrawColor(1);
      u8g2.setFontMode(0);

      //May need to pick a smaller font if you need more than 99999.99 digits
      //https://github.com/olikraus/u8g2/wiki/fntlistall
      //Font is 14 pixels high
      u8g2.setFont(u8g2_font_crox4hb_tf);
      //Centre output
      int width = u8g2.getStrWidth(s);
      u8g2.drawStr(LCD_WIDTH / 2 - width / 2, LCD_HEIGHT/2 + 14/2, s);
    }

    //Draw header inversed across top using 6 pixel high font
    //Font is 6 pixels high
    u8g2.setFont(u8g2_font_profont10_mr);
    u8g2.setFontMode(1);
    u8g2.drawBox(0, 0, LCD_WIDTH, 6 + 2);
    u8g2.setDrawColor(2);

    //Right align clock in top right corner
    sprintf(header, "%02d:%02d", hour(), minute());
    u8g2.drawStr(LCD_WIDTH - 1 - u8g2.getStrWidth(header), 6 + 1, header);

    ftoa(header, temperature, 10);
    //header[4]='°';
    header[4]='C';
    header[5]=0x00;
    u8g2.drawStr(24, 6 + 1, header);

    if (maxpage != 0) {
      //Print current page in top left
      sprintf(header, "%02d", page);
      u8g2.drawStr(2, 6 + 1, header);

      //Display units underneath reading
      /*
       * Commented out for now as we dont know what units the values relate to.
      strcpy(header,"kW/h");
      u8g2.setDrawColor(1);
      u8g2.setFontMode(0);
      u8g2.drawStr(LCD_WIDTH/2 - u8g2.getStrWidth(header)/2, LCD_HEIGHT/2 + 14+2 , header);
      */
    }

    u8g2.sendBuffer();
  }

  //Short delay
  delay(10);
  counter--;
}

void outputReading(char* s, float value, int offset) {
  //Buffer...
  //char s[10] = "          ";

  //  lc1.clearDisplay(0);

  float floored = floor(value);
  float remainder = value - floored;

  int decimaloffset = -1;

  if (remainder != 0)  {
    ftoa(s, value, 100);
  } else {
    itoa(floored, s, 10);
  }

}

char *ftoa(char *a, double f, long p)
{
  //Copied from https://forum.arduino.cc/index.php?topic=59027.0
  //tweaked to force 2 DP and fix bug

  //long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};

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
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p));
  itoa(desimal, a, 10);
  return ret;
}
