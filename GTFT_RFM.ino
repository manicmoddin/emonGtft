/***************************************************
 * This is our library for the Adafruit HX8357D Breakout
 * ----> http://www.adafruit.com/products/2050
 * 
 * Check out the links above for our tutorials and wiring diagrams
 * These displays use SPI to communicate, 4 or 5 pins are required to
 * interface (RST is optional)
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 * 
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include "JeeLib.h"
#include <avr/pgmspace.h>
#include <RTClib.h>                 // Real time clock (RTC) - used for software RTC to reset kWh counters at midnight
#include <Wire.h>                   // Part of Arduino libraries - needed for RTClib
#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)
//#includFreeSans9pt7b.h	
RTC_Millis RTC;



// These are 'flexible' lines that can be changed
#define TFT_CS 8
#define TFT_DC 7
#define TFT_RST -1 // RST can be set to -1 if you tie it to Arduino's reset

//define the colours that will be used.
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

//--------------------------------------------------------------------------------------------
// RFM12B Settings
//--------------------------------------------------------------------------------------------
#define MYNODE 20            // Should be unique on network, node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210            // network group, must be same as emonTx and emonBase

//---------------------------------------------------
// Data structures for transfering data between units

typedef struct { 
  int power1, emonBattery, emonTemp, power2; 
} 
PayloadTX;         // neat way of packaging data for RF comms
PayloadTX emontx;

typedef struct { 
  int temperature, light; 
} 
PayloadGLCD;
PayloadGLCD emonglcd;

typedef struct { 
  int temperature, light; 
} 
PayloadGTFT;
PayloadGTFT emongtft;

typedef struct { 
  int topTank, bottomTank, tankBattery; 
} 
PayloadTank;
PayloadTank emontank;

typedef struct { 
  int seconds, mins, hours; 
} 
PayloadBase;
PayloadBase emonBaseStation;


//Button Space
const int buttonSpaceH = 115;
const int buttonWidth = 100;
const int buttonHeight = 50;
const int buttonVSpacing = 11;
const int buttonHSpacing = 10;

//emonGLCD SETUP
const int maxGen = 3000;       //Peak output, can be used to set colours, icons etc
const int PVgenOffset = 70;    //once lower than this, assume the Invertor is doing noothing
const int backlight = 3;

//emonGTFT Variables
int hour = 8, minute = 0;      //setup a time that the display will be at on first boot.
int oldHour = 0, oldMinute = 0;
int last_hour = hour;
int last_minute = minute;
double usekwh = 0, genkwh = 0; //defaults for used and gen kwh.
int cval_use, cval_gen, old_use, old_gen;        //I believe these hold the "curved" values for the used and gen.
byte page = 1;
byte oldPage = 0;

//Temperature Sensor Setup
#define ONE_WIRE_BUS 4              // temperature sensor connection - hard wired 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double temp, maxtemp, mintemp;
double oldTemp = 0;;

//---------------------------------- 
// Flow control
//-------------------------------------------------------------------------------------------- 
unsigned long last_emontx;                   // Used to count time from last emontx update
unsigned long last_emonbase;                 // Used to co

//unsigned long fast_update, slow_update;

int changeMe = 0;
int oldChangeMe = 0;

int grid = 0;
int oldGrid = 0;

int solar = 0;
int oldSolar = 0;

int consumption = 0;
int oldConsumption = 0;

int tankTop = 0;
int tankBottom = 0;

int oldTankTop, oldTankBottom;
unsigned long interval = 300;
unsigned long slowUpdate;
unsigned long previousMillis = 0;


//---------------------------------- 
// LED Setup
//--------------------------------------------------------------------------------------------

const int redLed = 5;
const int greenLed = 6;
const int blueLed = 9;
const int LDRpin = A0;

void setup() {
  //analogWrite(3, 10);
  Serial.begin(9600);

  delay(500); 				   //wait for power to settle before firing up the RF
  rf12_initialize(MYNODE, freq,group);
  delay(100);				   //wait for RF to settle befor turning on display
  
  sensors.begin();                         // start up the DS18B20 temp sensor onboard  
  sensors.requestTemperatures();
  temp = (sensors.getTempCByIndex(0));     // get inital temperture reading
  mintemp = temp; maxtemp = temp;          // reset min and max
  
  tft.begin(HX8357D);
  tft.setRotation(1);
  clearScreen();
  //drawCrossHair();
  //drawButtons(3);
  //setupSolarMonitor();
  //setupWaterPage();

  pinMode(backlight, OUTPUT);
  
  pinMode(redLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  
  digitalWrite(backlight,1);
  digitalWrite(redLed, 1);
  delay(100);
  digitalWrite(blueLed, 1);
  delay(100);
  digitalWrite(greenLed, 1);
  delay(100);
  digitalWrite(redLed, 0);
  delay(100);
  digitalWrite(blueLed, 0);
  delay(100);
  digitalWrite(greenLed, 0);
  //test();
}


void loop(void) {

  if(Serial.available()) {
    handleInput(Serial.read());
  }
  

  unsigned long currentMillis = millis();

  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);
      if (node_id == 10) {
        emontx = *(PayloadTX*) rf12_data; 
        last_emontx = millis();
      }
      if (node_id == 11) {
        emontank = *(PayloadTank*) rf12_data;
      }
      if (node_id == 30) {
        emonglcd = *(PayloadGLCD*) rf12_data;
      }
      //if (node_id == 15) {emonBaseStation = *(PayloadBase*) rf12_data;}

      if (node_id == 15)
      {
        RTC.adjust(DateTime(2013, 1, 1, rf12_data[1], rf12_data[2], rf12_data[3]));
        //  weather = rf12_data[4];   ////////////// ADDED Weather Stuff HERE
        last_emonbase = millis();
      } 
    }
  }

  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    old_gen = cval_gen;
    old_use = cval_use;
    oldGrid = grid;
    grid = cval_use - cval_gen;
    //oldTemp = temp;
    //temp = emonglcd.temperature / 100;
    oldTankTop = tankTop;
    tankTop = emontank.topTank / 100;
    oldTankBottom = tankBottom;
    tankBottom = emontank.bottomTank /100;
    cval_use = cval_use + (emontx.power1 - cval_use)*0.50;
    //cval_gen = random(100, 5600);
    cval_gen = cval_gen + (emontx.power2 - cval_gen)*0.50;
    

    if (cval_gen<PVgenOffset) {
      cval_gen=0;                  //set generation to zero when generation level drops below a certian level (at night) eg. 20W
      //page = 3;
    }

    //if (hour >= 23 || hour < 6) {
    //  digitalWrite(backlight, 0);
    //}
    //else {
    //  digitalWrite(backlight, 1);
    //}

    if (page == 1) {
      if (cval_gen == 0) {
        page = 3;
        //drawGridMonitor(cval_use, old_use, temp, oldTemp);
      }
      
      else {
        drawSolarMonitor(cval_gen, old_gen, cval_use, old_use, grid, oldGrid, temp, oldTemp, oldChangeMe, changeMe);
      }
    }

    if (page == 2) {
      drawWaterPage(tankTop, tankBottom);
    }

    if (page == 3) {
      if (cval_gen > PVgenOffset) {
        page = 1;
      }
      else {
        drawGridMonitor(cval_use, old_use, temp, oldTemp);
      }
    }

    DateTime now = RTC.now();
    last_hour = hour;
    last_minute = minute;
    hour = now.hour();
    minute = now.minute();
    //oldHour = hour;
    writeTime();
    
    int LDR = analogRead(LDRpin);                     // Read the LDR Value so we can work out the light level in the room.
    int LDRbacklight = map(LDR, 0, 1023, 50, 250);    // Map the data from the LDR from 0-1023 (Max seen 1000) to var GLCDbrightness min/max
    LDRbacklight = constrain(LDRbacklight, 0, 255);   // Constrain the value to make sure its a PWM value 0-255
    if ((hour > 22) ||  (hour < 5)) digitalWrite(backlight,0); else analogWrite(backlight,LDRbacklight);  

    int PWRleds= map(cval_use-cval_gen, 0, 3000, 0, 255);     // Map importing value from (LED brightness - cval3 is the smoothed grid value - see display above 
    if (PWRleds<0) PWRleds = PWRleds*-1;                        // keep it positive 
    PWRleds = constrain(PWRleds, 0, 255);                       // Constrain the value to make sure its a PWM value 0-255
   
    if (cval_gen>PVgenOffset) {
      if (cval_gen > cval_use) {            //show green LED when gen>consumption cval are the smooth curve values  
	analogWrite(redLed, 0);         
	analogWrite(greenLed, PWRleds);    
        
      } else {                              //red if consumption>gen
        analogWrite(greenLed, 0); 
	analogWrite(redLed, PWRleds);   
      }
    } else {                                //Led's off at night and when solar PV is not generating
      analogWrite(redLed, 0);
      analogWrite(greenLed, 0);
    }
    //analogWrite(6, 5);
  }
  
  //slow update, temperature etc...
  if ((millis()-slowUpdate)>20000)
  {
    Serial.println("POW!!!");
    slowUpdate = millis();
    oldTemp = temp;
    int LDR = analogRead(LDRpin);
    sensors.requestTemperatures();
    double rawtemp = (sensors.getTempCByIndex(0));
    if ((rawtemp>-20) && (rawtemp<50)) temp=rawtemp;                  //is temperature withing reasonable limits?
    if (temp > maxtemp) maxtemp = temp;
    if (temp < mintemp) mintemp = temp;
   
    emongtft.temperature = (int) (temp * 100);                       // set emonglcd payload
    emongtft.light = 255;
    rf12_sendNow(0, &emonglcd, sizeof emonglcd);                     //send temperature data via RFM12B using new rf12_sendNow wrapper -glynhudson
    rf12_sendWait(2);    
  }
  
}

void test() {
  tft.drawPixel(random(0,tft.width()),random(0,tft.height()),random(0x0000, 0xFFFF));
}

void drawButtons(int noOfButtons) {

  int buttonY = 0 + buttonVSpacing;
  int buttonX = (tft.width() - buttonSpaceH) + buttonHSpacing;

  for(int i=0; i<noOfButtons; i++) {
    tft.drawRoundRect(buttonX-1, buttonY-1, buttonWidth, buttonHeight, 5, 0xffff);
    tft.fillRoundRect(buttonX, buttonY, buttonWidth, buttonHeight, 5, 0x001f);
    Serial.print(buttonX); 
    Serial.print(","); 
    Serial.println(buttonY);

    //move buttonY to the right point next
    buttonY = buttonY + buttonHeight + buttonVSpacing;
  }
  writeOnButtons();
}

void drawCrossHair() {
  tft.fillScreen(0x0000);
  //tft.drawPixel(0,0,0xffff);
  tft.drawLine(0, tft.height()/2, tft.width()-buttonSpaceH, tft.height()/2, 0xFFFF);                  //Draw Horizontal Line
  tft.drawLine((tft.width()-buttonSpaceH)/2, 0, (tft.width()-buttonSpaceH)/2, tft.height(), 0xFFFF);  //Draw Vertical Line
  //tft.drawLine((tft.width()-buttonSpaceH), 0, (tft.width()-buttonSpaceH), tft.height(), 0xFFFF);      //Draw Horizontal Line at Right hand side of display
}

void writeOnButtons() {
  tft.setCursor(tft.width() - (buttonSpaceH - buttonHSpacing*3), buttonHeight /2 + buttonVSpacing / 2);
  tft.setTextColor(0x0000);
  tft.setTextSize(2);
  tft.print("Energy");
  tft.setCursor(tft.width() - (buttonSpaceH - buttonHSpacing*3)-2, ((buttonHeight /2) + (buttonVSpacing / 2)) -2) ;
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.print("Energy");

  tft.setCursor(tft.width() - (buttonSpaceH - buttonHSpacing*3), (buttonHeight /2 + buttonVSpacing / 2) *3);
  tft.setTextColor(0x0000);
  tft.setTextSize(2);
  tft.print("Water");
  tft.setCursor(tft.width() - (buttonSpaceH - buttonHSpacing*3)-2, (buttonHeight /2 + buttonVSpacing / 2) *3 -2) ;
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.print("Water");

  tft.setCursor(tft.width() - (buttonSpaceH - buttonHSpacing*3), (buttonHeight /2 + buttonVSpacing / 2) *5);
  tft.setTextColor(0x0000);
  tft.setTextSize(2);
  tft.print("Night");
  tft.setCursor(tft.width() - (buttonSpaceH - buttonHSpacing*3)-2, (buttonHeight /2 + buttonVSpacing / 2) *5 -2) ;
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.print("Night");
}

void stopInterrupt() {
  //Serial.println("Stopping");
  detachInterrupt(0);
}

void startInterrupt() {
  //Serial.println("Starting");
  rf12_initialize(MYNODE, freq,group);
  //delay(100);
}

static void handlePage (char c) {
  if ('0' <= c && c <= '9') {
    switch(c) {

    case '1':
      page = 1;
      break;

    case '2':
      page = 2;
      break;
      
    case '3':
      page = 3;
      break;
    }
  }
}

static void handleInput (char c) {
  if ('0' <= c && c <= '9') {
    switch(c) {

    case '0':
      analogWrite(3,0);
      break;

    case '1':
      analogWrite(3,2);
      break;

    case '2':
      analogWrite(3,10);
      break;

    case '3':
      analogWrite(3,25);
      break;

    case '4':
      analogWrite(3,50);
      break;

    case '5':
      analogWrite(3,100);
      break;

    case '6':
      analogWrite(3,175);
      break;

    case '7':
      analogWrite(3,255);
      break;
    }
  }
}

void writeTime() {
  //Serial.println(last_hour);
  char str[10];
  char str2[10];
  if(hour != last_hour || minute != last_minute || page != oldPage) {
    stopInterrupt();
    tft.setTextSize(3);

    //black out the old value
    tft.fillRect(0,tft.height() - 25, 100, 25, BLACK);
    
    itoa((int)hour,str,10);
    if  (minute<10) strcat(str,":0"); 
    else strcat(str,":"); 
    itoa((int)minute,str2,10);
    strcat(str,str2); 
    tft.setTextColor(WHITE);
    tft.setCursor(0,tft.height() -25);
    tft.print(str);
 
    //    //    //black out the old value
    //    //    tft.setCursor(0,tft.height() -25);
    //    //    tft.setTextColor(0x0000);
    //    //    //tft.print(last_hour);
    //    //    //tft.print(":");
    //    //    //tft.print(minute);
    //    tft.fillRect(0,tft.height() - 25, 100, 25, BLACK);
    //
    //    //now print the new value
    //    tft.setTextColor(WHITE);
    //    tft.setCursor(0,tft.height() -25);
    //    tft.print(hour);
    //    tft.print(":");
    //    tft.print(minute);
    //    startInterrupt();
    //  }
    //
    //  if(minute != last_minute) {
    //    stopInterrupt();
    //    tft.setTextSize(3);
    //    tft.fillRect(0,tft.height() - 25, 100, 25, BLACK);
    //
    //    //now print the new value
    //    tft.setTextColor(WHITE);
    //    tft.setCursor(0,tft.height() -25);
    //    tft.print(hour);
    //    tft.print(":");
    //    tft.print(minute);
    startInterrupt();    
  }
}


