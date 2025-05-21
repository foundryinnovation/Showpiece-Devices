#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

//#include "SPI.h"
//#include "Adafruit_GFX.h"
//#include "Adafruit_ILI9341.h"
//#include "XPT2046_Touchscreen.h"
#include "Math.h"

/*
// For the Adafruit shield, these are the default.
#define TFT_CS 10
#define TFT_DC 9
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 8
#define TFT_MISO 12

#define TS_CS 7
*/

#define ROTATION 3


// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC/RST
//Adafruit_TFT tft = Adafruit_TFT(TFT_CS, TFT_DC, TFT_RST);
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen ts(T_CS, T_IRQ);
//XPT2046_Touchscreen ts(TS_CS);

// calibration values
float xCalM = 0.0, yCalM = 0.0; // gradients
float xCalC = 0.0, yCalC = 0.0; // y axis crossing points

int8_t blockWidth = 20; // block size
int8_t blockHeight = 20;
int16_t blockX = 0, blockY = 0; // block position (pixels)

class ScreenPoint {
  public:
  int16_t x;
  int16_t y;

// default constructor
ScreenPoint(){}

ScreenPoint(int16_t xIn, int16_t yIn){
  x = xIn;
  y = yIn;
}

};

ScreenPoint getScreenCoords(int16_t x, int16_t y){
  int16_t xCoord = round((x * xCalM) + xCalC);
  int16_t yCoord = round((y * yCalM) + yCalC);
  if(xCoord < 0) xCoord = 0;
  if(xCoord >= tft.width()) xCoord = tft.width() - 1;
  if(yCoord < 0) yCoord = 0;
  if(yCoord >= tft.height()) yCoord = tft.height() - 1;
  return(ScreenPoint(xCoord, yCoord));
}

void calibrateTouchScreen(){
  TS_Point p;
  int16_t x1,y1,x2,y2;

  tft.fillScreen(TFT_BLACK);
  // wait for no touch
  while(ts.touched());
  tft.drawFastHLine(10,20,20,TFT_RED);
  tft.drawFastVLine(20,10,20,TFT_RED);
  while(!ts.touched());
  delay(50);
  p = ts.getPoint();
  x1 = p.x;
  y1 = p.y;
  tft.drawFastHLine(10,20,20,TFT_BLACK);
  tft.drawFastVLine(20,10,20,TFT_BLACK);
  delay(500);
  while(ts.touched());
  tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,TFT_RED);
  tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,TFT_RED);
  while(!ts.touched());
  delay(50);
  p = ts.getPoint();
  x2 = p.x;
  y2 = p.y;
  tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,TFT_BLACK);
  tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,TFT_BLACK);

  int16_t xDist = tft.width() - 40;
  int16_t yDist = tft.height() - 40;

  // translate in form pos = m x val + c
  // x
  xCalM = (float)xDist / (float)(x2 - x1);
  xCalC = 20.0 - ((float)x1 * xCalM);
  // y
  yCalM = (float)yDist / (float)(y2 - y1);
  yCalC = 20.0 - ((float)y1 * yCalM);

  Serial.print("x1 = ");Serial.print(x1);
  Serial.print(", y1 = ");Serial.print(y1);
  Serial.print("x2 = ");Serial.print(x2);
  Serial.print(", y2 = ");Serial.println(y2);
  Serial.print("xCalM = ");Serial.print(xCalM);
  Serial.print(", xCalC = ");Serial.print(xCalC);
  Serial.print("yCalM = ");Serial.print(yCalM);
  Serial.print(", yCalC = ");Serial.println(yCalC);

}


void setup() {
  Serial.begin(9600);

// avoid chip select contention
/*
pinMode(TS_CS, OUTPUT);
digitalWrite(TS_CS, HIGH);
pinMode(TFT_CS, OUTPUT);
digitalWrite(TFT_CS, HIGH);
*/
  tft.init();
  tft.setRotation(ROTATION);
  tft.fillScreen(TFT_BLACK);

  touchscreenSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, T_CS);
  ts.begin(touchscreenSPI);

  ts.setRotation(ROTATION);
  calibrateTouchScreen();

}

void moveBlock(){
  int16_t newBlockX, newBlockY;
  ScreenPoint sp = ScreenPoint();
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    sp = getScreenCoords(p.x, p.y);
    newBlockX = sp.x - (blockWidth / 2);
    newBlockY = sp.y - (blockHeight / 2);
    if (newBlockX < 0) newBlockX = 0;
    if (newBlockX >= (tft.width() - blockWidth)) newBlockX = tft.width() - 1 - blockWidth;
    if (newBlockY < 0) newBlockY = 0;
    if (newBlockY >= (tft.height() - blockHeight)) newBlockY = tft.height() - 1 - blockHeight;
  }

  if ((abs(newBlockX - blockX) > 2) || (abs(newBlockY - blockY) > 2)){
    tft.fillRect(blockX, blockY, blockWidth, blockHeight,TFT_BLACK);
    blockX = newBlockX;
    blockY = newBlockY;
    tft.fillRect(blockX, blockY, blockWidth, blockHeight,TFT_RED);
  }
}

unsigned long lastFrame = millis();

void loop(void) {

  // limit frame rate
  while((millis() - lastFrame) < 20);
  lastFrame = millis();

  if (ts.touched()) {
    moveBlock();
  }

}

