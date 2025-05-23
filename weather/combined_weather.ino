//#include <WiFi.h>
//#include <HTTPClient.h>
//#include <WiFiClientSecure.h>
//#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>


typedef void (*WindowDraw)(); //for making a list of function pointers; respresents drawing the window

typedef void (*WindowEvent)(); //for making a list of function pointers

typedef struct WindowObject {
  WindowDraw drawFunction;
  WindowEvent eventFunction;

  WindowObject(WindowDraw drawer, WindowEvent eventer) : drawFunction(drawer), eventFunction(eventer) {}
} WindowObject;

typedef struct CalibrateOffset {
  float xCalM;
  float xCalC;
  float yCalM;
  float yCalC;
} CalibrateOffset;

typedef struct ScreenPoint {
  int x;
  int y;
} ScreenPoint;

ScreenPoint getPoint();
boolean isTouched();
void drawBall();
void drawWindowOne();
void drawWindowTwo();

//wifi
const char* SSID = "Innovation_Foundry";
const char* PASSWORD = "@Innovate22";

//windows
std::vector<WindowObject> windows;
WindowObject* currentWindow;

//touchscreen
TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(T_CS, T_IRQ);
const CalibrateOffset offsets{-0.13, 502, 0.09, -27.60};
int tempstate = 0;

ScreenPoint getPoint(){
  TS_Point p = touchscreen.getPoint();
  int16_t xCoord = round((p.x * offsets.xCalM) + offsets.xCalC);
  int16_t yCoord = round((p.y * offsets.yCalM) + offsets.yCalC);

  if(xCoord < 0) xCoord = 0;
  if(xCoord >= tft.width()) xCoord = tft.width() - 1;
  if(yCoord < 0) yCoord = 0;
  if(yCoord >= tft.height()) yCoord = tft.height() - 1;

  return (ScreenPoint){xCoord, yCoord};
}

boolean isTouched(){
  return touchscreen.tirqTouched() && touchscreen.touched();
}

void drawBall(){
    Serial.print("Touched ");
    ScreenPoint p = getPoint();

    Serial.print(p.x);
    Serial.print(" ");
    Serial.print(p.y);
    Serial.println();
    tft.fillCircle(p.x,p.y,2,0xFF0000);
}

void drawWindowOne(){
  Serial.println("draw: in window 1");
  tft.fillScreen(TFT_RED);
}

void eventWindowOne(){
  Serial.println("event: in window 1");
  if (isTouched()) {
    tempstate++;
    currentWindow = &windows.at(tempstate % windows.size());
    currentWindow->drawFunction();
    //drawBall();
    delay(100);
  }
}

void drawWindowTwo(){
  Serial.println("draw: in window 2");
  tft.fillScreen(TFT_BLUE);
}

void eventWindowTwo(){
  Serial.println("event: in window 2");
  if (isTouched()) {
    drawBall();
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //touchscreen
  tft.init();
  tft.setRotation(1);
  touchscreenSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, T_CS);
  touchscreen.begin(touchscreenSPI);
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  //windows
  windows.push_back(WindowObject(drawWindowOne, eventWindowOne));
  windows.push_back(WindowObject(drawWindowTwo, eventWindowTwo));

  currentWindow = &windows.at(0);
  currentWindow->drawFunction();
}

void loop() {
  currentWindow->eventFunction();
}

