#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

/* forward declarations types */

typedef void (*WindowDraw)(); //for making a list of function pointers; respresents drawing the window
typedef void (*WindowEvent)(); //for making a list of function pointers

typedef struct ScreenPoint ScreenPoint;
typedef struct CalibrateOffset CalibrateOffset;
typedef struct WindowObject WindowObject;
typedef struct APIObject APIObject;

/* forward declaration functions */

//wifi
bool checkWifi();

//internal window stuff
//TODO: move to header file 
void drawStatusBox(const char*, uint8_t);

//windows
void drawWindowOne();
void drawWindowTwo();
void eventWindowOne();
void eventWindowTwo();

//touchscreen
ScreenPoint getPoint();
bool isTouched();
void drawBall();


/* types */

struct WindowObject {
  WindowDraw drawFunction;
  WindowEvent eventFunction;

  WindowObject(WindowDraw drawer, WindowEvent eventer) : drawFunction(drawer), eventFunction(eventer) {}
};

struct CalibrateOffset {
  float xCalM;
  float xCalC;
  float yCalM;
  float yCalC;
};

struct ScreenPoint {
  int x;
  int y;
};

struct APIObject {
  const char* url;
  JsonDocument doc;
  
  APIObject() {}

  APIObject(const char* url){
    this->url = url;
    updateData();
  }
  
  void updateData(){
    Serial.println("Updating api data");
    checkWifi();

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;

    if (!https.begin(client, this->url)) {
        Serial.println("HTTPS begin failed");
        drawStatusBox("Failed to get API", 2);
        return;
    }

    int httpCode = https.GET();
    if (httpCode <= 0) {
        Serial.printf("HTTP GET failed, error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        return;
    }

    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      https.end();

      Serial.print(payload);

      auto error = deserializeJson(doc, payload);
      if(error){
        Serial.println("deserialization failed");
        return;
      }

    }else{
      Serial.printf("HTTP GET failed, error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        return;
    }
  }
};


/* variables */

//wifi
const char* SSID = "Innovation_Foundry";
const char* PASSWORD = "@Innovate22";

const char* WEATHER_HOST = "api.open-meteo.com";
const char* WEATHER_PATH = "/v1/forecast?latitude=40.7146&longitude=-74.3646&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timezone=America%2FNew_York";

APIObject weatherAPI;

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

bool isTouched(){
  return touchscreen.tirqTouched() && touchscreen.touched();
}

//TODO: delete
void drawBall(){
    Serial.print("Touched ");
    ScreenPoint p = getPoint();

    Serial.print(p.x);
    Serial.print(" ");
    Serial.print(p.y);
    Serial.println();
    tft.fillCircle(p.x,p.y,2,0xFF0000);
}

bool checkWifi(){
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    drawStatusBox("Wifi reconnecting...", 2);
    WiFi.reconnect();
    delay(5000);
    
    if (WiFi.status() != WL_CONNECTED) {
      drawStatusBox("Wifi Failed to Reconnect", 2);
      return false;
    }else {
      return true;
    }
  }
}

void drawStatusBox(const char* text, uint8_t size){
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(size); 

  tft.drawString(text, 0, 0);
}

void drawWindowOne(){
  Serial.println("draw: in window 1");
  Serial.printf("weather api %s\n", weatherAPI.doc["current"]["temperature_2m_max"][0]);
}

void eventWindowOne(){
  //Serial.println("event: in window 1");

  /*
  if (isTouched()) {
    tempstate++;
    currentWindow = &windows.at(tempstate % windows.size());
    currentWindow->drawFunction();
    //drawBall();
    delay(100);
  }
  */
}

void drawWindowTwo(){
  Serial.println("draw: in window 2");
  /*
  tft.fillScreen(TFT_BLUE);
  */
}

void eventWindowTwo(){
  //Serial.println("event: in window 2");
  /*
  if (isTouched()) {
    drawBall();
  }
  */
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


  //wifi
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println(" Connected!");
    delay(1000);
  }else{
    Serial.println(" Failed to connect!");
    delay(3000);
    ESP.restart();
  }

  weatherAPI = APIObject("https://api.open-meteo.com/v1/forecast?latitude=40.7146&longitude=-74.3646&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timezone=America%2FNew_York");
  
  //init window
  currentWindow = &windows.at(0);
  currentWindow->drawFunction();
}

void loop() {
  currentWindow->eventFunction();
}

