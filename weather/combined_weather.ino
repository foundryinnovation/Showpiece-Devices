//#include <WiFi.h>
//#include <HTTPClient.h>
//#include <WiFiClientSecure.h>
//#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>


typedef void (*WindowDraw)(); //for making a list of function pointers

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
void drawWindowThree();

//wifi
const char* SSID = "Innovation_Foundry";
const char* PASSWORD = "@Innovate22";

//windows
std::vector<WindowDraw> windows;

//touchscreen
TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(T_CS, T_IRQ);
CalibrateOffset offsets{ -0.13, 527.99, 0.09, -7.60};



ScreenPoint getPoint(){
  TS_Point p = touchscreen.getPoint();
  int16_t xCoord = round((p.x * offsets.xCalM) + offsets.xCalC);
  int16_t yCoord = round((p.y * offsets.yCalM) + offsets.yCalC);

  if(xCoord < 0) xCoord = 0;
  if(xCoord >= tft.width()) xCoord = tft.width() - 1;
  if(yCoord < 0) yCoord = 0;
  if(yCoord >= tft.height()) yCoord = tft.height() - 1;

  Serial.print("IN GETPOINT");
  Serial.print(xCoord);
  Serial.print(" ");
  Serial.print(yCoord);
  Serial.println();

  return (ScreenPoint){xCoord, yCoord};
}

boolean isTouched(){
  return touchscreen.tirqTouched() && touchscreen.touched();
}

void drawBall(){
    Serial.print("Touched ");
    ScreenPoint p = getPoint();
    //ScreenPoint p;
    //p.x = map(sp.x, 200, 3700, 1, 480);
    //p.y = map(sp.y, 240, 3800, 1, 320);

    Serial.print(p.x);
    Serial.print(" ");
    Serial.print(p.y);
    Serial.println();
    tft.fillCircle(p.x,p.y,2,0xFF0000);
}

void drawWindowOne(){
  Serial.println("Rat");
}

void drawWindowTwo(){
  Serial.println("Hamburger");
}


void drawWindowThree(){
  Serial.println("Garbonzo beans");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);

  touchscreenSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, T_CS);
  touchscreen.begin(touchscreenSPI);

  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);


  windows.push_back(drawWindowOne);
  windows.push_back(drawWindowTwo);
}

void loop() {
  // put your main code here, to run repeatedly:
  /*
  Serial.println("Doing for loop");
  for(int i = 0; i < 2; i++){
    Serial.print("i = ");
    Serial.print(i);
    Serial.println();
  
    windows.at(i)();
    delay(100);
  }
  delay(2000);
  */
  if (isTouched()) {
    Serial.print("REGULAR POINT ");
    TS_Point p = touchscreen.getPoint();
    Serial.print(p.x);
    Serial.print(" ");
    Serial.print(p.y);
    Serial.println();
    drawBall();
  }
}

