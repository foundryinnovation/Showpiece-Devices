//#include <WiFi.h>
//#include <HTTPClient.h>
//#include <WiFiClientSecure.h>
//#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>


typedef void (*WindowDraw)(); //for making a list of function pointers

struct CoolPoint {
  int x;
  int y;
}

void drawWindowOne(){
  Serial.println("Rat");
}

void drawWindowTwo(){
  Serial.println("Hamburger");
}

const char* SSID = "Innovation_Foundry";
const char* PASSWORD = "@Innovate22";
std::vector<WindowDraw> windows;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  windows.push_back(drawWindowOne);
  windows.push_back(drawWindowTwo);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Doing for loop");
  for(int i = 0; i < 2; i++){
    Serial.print("i = ");
    Serial.print(i);
    Serial.println();
  
    windows.at(i)();
    delay(100);
  }
  delay(2000);
}

