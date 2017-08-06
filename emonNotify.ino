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

#define RF69_COMPAT 1                                                    // Set to 1 if using RFM69CW or 0 is using RFM12B

#define DEBUG 1

#define EMON_NOTIFY_DISPLAY_NUMBER 0        //Multiple display may be possible in the future with different outputs
#define RF_freq RF12_433MHZ                                              // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.

//Time between pages in 10ms counts (500= 5 seconds)
#define TIME_BETWEEN_PAGES 500;

//Time between reading one-wire temperature (30 seconds)
#define TIME_BETWEEN_TEMP_READINGS 3000;

#define MAXIMUM_READING_BUFFER 10
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

int32_t reading[MAXIMUM_READING_BUFFER];
char feedlabel[MAXIMUM_READING_BUFFER][15];

int page = 1;
int maxpage = 0;

int temp_counter = 0;

int counter = 0;

/*
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
*/

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


void setup() {
  // put your setup code here, to run once:
  memset(reading, 0, sizeof(reading));

  //Seed clock to a sensible date before we receive a reading from emonHUB
  setTime(1501545600);

#if DEBUG
  Serial.begin(38400);  Serial.println("emonNOTIFY");
#endif

  // Set Backlight Intensity
  // 0 is off
  // 255 is on
  analogWrite(BACKLIGHT_LED_PIN, 0);

  u8g2.begin();

  sensors.begin();
  tempSensors = sensors.getDeviceCount();
  if (tempSensors != 0) {
    sensors.getAddress(address_T1, 0);                  //get the address of the first detected sensor
  }

  //initialize RFM12B/rfm69CW
  rf12_initialize(nodeID, RF_freq, networkGroup, RF_FrequencyCorrection );

  //test();
}

//Display buffer
//char s[20];
char charbuffer[40];

int temperature;

void process_packet() {

#if DEBUG
  analogWrite(BACKLIGHT_LED_PIN, 255);
#endif

  int i = 0;
  int x = 0;


  //First byte is a header - is this for our display?
  uint8_t device = (rf12_data[i] & 0xF0) >> 4;

#if DEBUG
  Serial.print("device="); Serial.println(device);
#endif
  //If this message is not for me, quit out
  if (device != EMON_NOTIFY_DISPLAY_NUMBER) return;

  uint8_t message_type = rf12_data[i] & 0x0F;
  i++;

#if DEBUG
  Serial.print("type="); Serial.println(message_type);
#endif


  if (message_type == 0) {
    // New set of readings/feed values...
    // Clear existing array
    memset(reading, 0, sizeof(reading));
    while (i < rf12_len && x < MAXIMUM_READING_BUFFER ) {

      //4 bytes are the date time in EPOCH format
      if (x == 0) {
        //TIME
        setTime(Read4ByteInt32(rf12_data, i));
      } else {
        reading[x] = Read4ByteInt32(rf12_data, i);
#if DEBUG
        Serial.print("value="); Serial.println(reading[x]);
#endif
      }

      i += sizeof(int32_t);
      x++;
    }

    maxpage = x;
    return;
  }

  if (message_type == 1) {
    //Its a label....
    uint8_t sequence = rf12_data[i++];

    #if DEBUG
    Serial.print("seq=");
    Serial.println(sequence);
    #endif

    while (i < rf12_len) {
      Serial.print(rf12_data[i], HEX);
      Serial.print(' ');
      i++;      
    }
    Serial.println();

    strcpy(feedlabel[sequence], "STUART");

    //strncpy(feedlabel[sequence], rf12_data[i], sizeof(feedlabel[sequence]) );

    #if DEBUG
      //Serial.println(feedlabel[sequence]);
    #endif
    return;
  }

}

void loop() {

  if (tempSensors != 0 && temp_counter == 0) {
    // Request Temperature from sensor
    sensors.requestTemperatures();
    //Temperature in Celcius multiplied by 100
    temperature = sensors.getTempC(address_T1) * 100;
    temp_counter = TIME_BETWEEN_TEMP_READINGS;
  }


  if (rf12_recvDone()) {    //if RF Packet is received
    if (rf12_crc == 0) {    //Check packet is good

      //Only allow packets addressed to me
      if ((rf12_hdr & RF12_HDR_MASK) == nodeID    ) {
        // Returns 1 if RF packet is received so we have just
        // received a fresh packet of data from emonPI
        process_packet();
      }
    }
  }

  if (counter == 0) {

    //Counter has reached zero, so refresh display  with new page of data
    analogWrite(BACKLIGHT_LED_PIN, 100);

    //Reset counter
    counter = TIME_BETWEEN_PAGES;

    //Move to next page
    page++;

    //Loop to start if needed (page 1 = first, zero means no data received yet)
    if (page >= maxpage) {
      page = 1;
    }

    draw_header(page);
    if (maxpage != 0) {
      draw_page(page, reading[page], feedlabel[page]);
    }
    paint();
  }

  //Short delay
  delay(10);
  counter--;
  temp_counter--;
}

void draw_header(uint8_t p) {
  //Clear in RAM screen buffer
  u8g2.clearBuffer();

  //Draw header inversed across top using 6 pixel high font
  //Font is 6 pixels high
  u8g2.setFont(u8g2_font_profont10_mr);
  u8g2.setFontMode(1);
  u8g2.drawBox(0, 0, LCD_WIDTH, 6 + 2);
  u8g2.setDrawColor(2);

  //Right align clock in top right corner
  sprintf(charbuffer, "%02d:%02d", hour(), minute());
  u8g2.drawStr(LCD_WIDTH - 1 - u8g2.getStrWidth(charbuffer), 6 + 1, charbuffer);

  //ftoa(header, temperature, 10);
  sprintf(charbuffer, "%02d.%d°C", temperature / 100, temperature % 10);
  //header[4]='°';
  //header[4] = 'C';
  //header[5] = 0x00;
  u8g2.drawStr(24, 6 + 1, charbuffer);

  //Print current page in top left
  sprintf(charbuffer, "%02d", p);
  u8g2.drawStr(2, 6 + 1, charbuffer);
}

void draw_page(uint8_t p, int32_t value, char* label) {
  //Inputs p=page, value=reading value

  //Clear char buffer
  //memset(charbuffer, 0, sizeof(charbuffer));

  //Build up string of text
  outputReading(charbuffer, value, 0);

  //LCD is 84x48 pixels
  u8g2.setDrawColor(1);
  u8g2.setFontMode(0);

  //May need to pick a smaller font if you need more than 99999.99 digits
  //https://github.com/olikraus/u8g2/wiki/fntlistall
  //Font is 19 pixels high
  u8g2.setFont(u8g2_font_crox4hb_tn);

  //Centre output horizontally
  int width = u8g2.getStrWidth(charbuffer);

  if (width > LCD_WIDTH) {
    //Reading won't fit onto display, try smaller font
    u8g2.setFont(u8g2_font_crox2hb_tn);
    width = u8g2.getStrWidth(charbuffer);
  }

  u8g2.drawStr(LCD_WIDTH / 2 - width / 2, LCD_HEIGHT - 10, charbuffer);

  //Display units underneath reading

  //Commented out for now as we dont know what units the values relate to.
  u8g2.setFont(u8g2_font_profont11_mr);
  u8g2.setDrawColor(1);
  u8g2.setFontMode(0);

  //TODO Add in units - but these need to be transmitted from emonHUB
  //strcpy(charbuffer,"kW/h");
  //u8g2.drawStr(LCD_WIDTH/2 - u8g2.getStrWidth(charbuffer)/2, LCD_HEIGHT , charbuffer);

  u8g2.drawStr(LCD_WIDTH / 2 - u8g2.getStrWidth(label) / 2, 20 , label);
}

void paint() {
  //If the RFM module receives a packet at the same time as we try and update the display
  //the display contents will corrupt, so switch off interputs for this call
  noInterrupts();
  u8g2.sendBuffer();
  interrupts();
}


void outputReading(char* s, int32_t value, int offset) {
  //TODO: There are more code efficient SPRINTF routines available for Arduino
  //      consider replacing when memory footprint gets tight.

  int32_t v = value / 100;

  //Strip out negative numbers from fractional part
  if (value < 0) value *= -1;
  uint8_t mod100 = value % 100;

  s += sprintf(s, "%ld", v);

  if (mod100 != 0) {
    //Add in the decimal point
    s[0] = '.';
    s++;
    ///Print the remainder (unsigned)
    s += sprintf(s, "%u", mod100);
  }
}

