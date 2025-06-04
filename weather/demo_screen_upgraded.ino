#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <time.h>
#include <TJpg_Decoder.h>
#include <FS.h>
#include "LittleFS.h"
#include "List_LittleFS.h"
#include "Web_Fetch.h"

#define BOOT_PIN 0     // the side button

/* forward declarations types */

typedef void (*WindowDraw)();  //for making a list of function pointers; respresents drawing the window
typedef void (*WindowEvent)(); //for making a list of function pointers

typedef struct ScreenPoint ScreenPoint;
typedef struct CalibrateOffset CalibrateOffset;
typedef struct WindowObject WindowObject;
typedef struct APIObject APIObject;

/* forward declaration functions */

//wifi
bool checkWifi();

//internal window stuff
void drawStatusBox(const char*, uint8_t);
bool tft_output(int16_t , int16_t , uint16_t , uint16_t , uint16_t* );

//windows
void drawSmartMirrorWindow();
void drawWeatherWindow();
void drawJokeWindow();
void drawJokeFullWindow();
void eventSmartMirrorWindow();
void eventWeatherWindow();
void eventJokeWindow();
void eventJokeFullWindow();

void switchToWindow(int);
void switchToNextWindow();

//touchscreen
ScreenPoint getPoint();
bool isTouched();

//animation helpers
void fadeTransition();
void drawWeatherIcon(int x, int y, int weatherCode);
void drawAnimatedBorder();
String getWeatherDescription(float temp, int weatherCode);
String getMotivationalQuote();
String get12HourTime(int hour, int minute);

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
  uint32_t lastUpdate;
  uint32_t updateInterval;
  
  APIObject() {}

  APIObject(const char* url, uint32_t interval){
    this->url = url;
    this->lastUpdate = millis();
    this->updateInterval = interval;
  }
  
  void updateData(){
    Serial.println("Updating api data");
    if (!checkWifi()) return;

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
    this->lastUpdate = millis();
  }

  // This next function will be called during decoding of the jpeg file to
  // render each block to the TFT.  If you use a different TFT library
  // you will need to adapt this function to suit.
  
};


/* variables */

// NTP Time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5 * 3600;  // EST (adjust for your timezone)
const int   daylightOffset_sec = 3600;   // Daylight saving time

//wifi/apis
APIObject weatherAPI;
APIObject jokeAPI;
APIObject dogAPI;

// Joke display
String currentFullJoke = "";
bool showingFullJoke = false;
bool jokeTruncated = false;

//windows
std::vector<WindowObject> windows;
WindowObject* currentWindow;
int currentWindowIndex = 0;

//touchscreen - PRESERVED EXACTLY AS IS
TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(T_CS, T_IRQ);
const CalibrateOffset offsets{-0.13, 502, 0.09, -27.60};
int tempstate = 0;

//animation variables
float gradientOffset = 0;
unsigned long lastAnimationUpdate = 0;
const int ANIMATION_INTERVAL = 50;

// Auto-rotation variables
unsigned long lastScreenChange = 0;
unsigned long lastTouch = 0;
const unsigned long SCREEN_ROTATION_INTERVAL = 15000; // 15 seconds
bool autoRotateEnabled = true;


// Layout constants for consistent spacing
const int MARGIN = 20;
const int WEATHER_ICON_Y = 180;
const int WEATHER_DETAILS_Y = 235;
const int LINE_SPACING = 32;
const int INDICATOR_Y_OFFSET = 35; // Distance from bottom

// High contrast colors for smart mirror
// colors
const uint16_t PRIMARY_COLOR = TFT_WHITE;
const uint16_t SECONDARY_COLOR = TFT_CYAN;
const uint16_t BACKGROUND_COLOR = TFT_BLACK;
const uint16_t ACCENT_COLOR = TFT_MAGENTA;
const uint16_t VIBE_COLOR = TFT_GREENYELLOW;

// Motivational quotes
const char* quotes[] = {
  "Today is your day!",
  "You've got this!",
  "Make it amazing",
  "Shine bright today",
  "Great things ahead",
  "Stay positive!",
  "You're awesome!",
  "Dream big today"
};
const int numQuotes = 8;

// Days of week and months
const char* daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* monthNames[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// PRESERVED TOUCH FUNCTIONS EXACTLY AS IS
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
  Serial.println("screen touched!");
  return touchscreen.tirqTouched() && touchscreen.touched();
}

bool checkWifi(){
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(5000);
    
    if (WiFi.status() != WL_CONNECTED) {
      drawStatusBox("Wifi Failed", 2);
      return false;
    }
  }
  return true;
}


// ***for jpeg decoding***
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
    // Stop further decoding as image is running off bottom of screen
    if ( y >= tft.height() ) return 0;

    // This function will clip the image block rendering automatically at the TFT boundaries
    tft.pushImage(x, y, w, h, bitmap);

    // Return 1 to decode next block
    return 1;
}

void drawStatusBox(const char* text, uint8_t size){
  tft.fillRect(0, 0, tft.width(), 30, TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(size); 
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, tft.width()/2, 15);
  tft.setTextDatum(TL_DATUM);
}

String get12HourTime(int hour, int minute) {
  char timeStr[10];
  String ampm = "AM";
  
  if (hour == 0) {
    hour = 12;
  } else if (hour == 12) {
    ampm = "PM";
  } else if (hour > 12) {
    hour -= 12;
    ampm = "PM";
  }
  
  sprintf(timeStr, "%d:%02d", hour, minute);
  return String(timeStr) + " " + ampm;
}

String getMotivationalQuote() {
  return quotes[random(numQuotes)];
}

String getWeatherDescription(float temp, int weatherCode) {
  String desc = "";
  
  // Temperature description - keep it concise for centering
  if (temp >= 80) {
    desc = "Hot Day";
  } else if (temp >= 70) {
    desc = "Perfect Weather";
  } else if (temp >= 60) {
    desc = "Nice & Mild";
  } else if (temp >= 50) {
    desc = "Cool Day";
  } else if (temp >= 40) {
    desc = "Bundle Up";
  } else if (temp >= 32) {
    desc = "Cold Day";
  } else {
    desc = "Freezing!";
  }
  
  // Add weather emoji on new line for better centering
  return desc;
}

void drawWeatherEmoji(int weatherCode, int x, int y) {
  tft.setTextSize(3);
  tft.setTextDatum(TC_DATUM);
  
  // Since emojis don't render, use text representations
  if (weatherCode <= 3) {
    tft.setTextColor(TFT_YELLOW, BACKGROUND_COLOR);
    tft.drawString("SUNNY", x, y);
  } else if (weatherCode >= 51 && weatherCode <= 67) {
    tft.setTextColor(TFT_CYAN, BACKGROUND_COLOR);
    tft.drawString("RAINY", x, y);
  } else if (weatherCode >= 71 && weatherCode <= 77) {
    tft.setTextColor(TFT_WHITE, BACKGROUND_COLOR);
    tft.drawString("SNOWY", x, y);
  } else if (weatherCode >= 95) {
    tft.setTextColor(TFT_YELLOW, BACKGROUND_COLOR);
    tft.drawString("STORMY", x, y);
  } else {
    tft.setTextColor(TFT_LIGHTGREY, BACKGROUND_COLOR);
    tft.drawString("CLOUDY", x, y);
  }
  tft.setTextColor(PRIMARY_COLOR, BACKGROUND_COLOR); // Reset color
}

void drawAnimatedBorder() {
  // Rainbow effect
  uint16_t colors[] = {TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, TFT_GREENYELLOW};
  int colorIndex = (int)(gradientOffset * 4) % 4;
  uint16_t borderColor = colors[colorIndex];
  
  // Draw animated border
  for (int i = 0; i < 3; i++) {
    tft.drawRect(5 + i, 5 + i, tft.width() - 10 - (i*2), tft.height() - 10 - (i*2), borderColor);
  }
}

void drawWeatherIcon(int x, int y, int weatherCode) {
  tft.setTextSize(1);
  
  if (weatherCode <= 1) {
    // Sun with animation
    int sunRadius = 30 + (sin(gradientOffset * 3.14) * 5);
    tft.fillCircle(x, y, sunRadius, TFT_YELLOW);
    for (int i = 0; i < 12; i++) {
      float angle = i * PI / 6 + gradientOffset;
      int x1 = x + cos(angle) * (sunRadius + 10);
      int y1 = y + sin(angle) * (sunRadius + 10);
      int x2 = x + cos(angle) * (sunRadius + 20);
      int y2 = y + sin(angle) * (sunRadius + 20);
      tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
      tft.drawLine(x1+1, y1, x2+1, y2, TFT_YELLOW);
    }
  } else if (weatherCode <= 3) {
    // Partly cloudy
    tft.fillCircle(x-10, y-10, 20, TFT_YELLOW);
    tft.fillCircle(x, y, 25, TFT_WHITE);
    tft.fillCircle(x+20, y, 20, TFT_WHITE);
    tft.fillCircle(x-15, y+5, 20, TFT_WHITE);
  } else if (weatherCode >= 51 && weatherCode <= 67) {
    // Rain cloud
    tft.fillCircle(x, y-20, 25, TFT_DARKGREY);
    tft.fillCircle(x+20, y-20, 20, TFT_DARKGREY);
    tft.fillCircle(x-15, y-15, 20, TFT_DARKGREY);
    // Animated rain
    for (int i = 0; i < 5; i++) {
      int dropX = x - 20 + (i * 10);
      int dropY = y + 10 + ((int)(gradientOffset * 20 + i * 5) % 20);
      tft.fillCircle(dropX, dropY, 3, TFT_CYAN);
      tft.fillTriangle(dropX-3, dropY, dropX+3, dropY, dropX, dropY-8, TFT_CYAN);
    }
  } else if (weatherCode >= 71 && weatherCode <= 77) {
    // Snow cloud
    tft.fillCircle(x, y-20, 25, TFT_LIGHTGREY);
    tft.fillCircle(x+20, y-20, 20, TFT_LIGHTGREY);
    tft.fillCircle(x-15, y-15, 20, TFT_LIGHTGREY);
    // Animated snow
    for (int i = 0; i < 6; i++) {
      int flakeX = x - 25 + (i * 10);
      int flakeY = y + 10 + ((int)(gradientOffset * 15 + i * 3) % 15);
      tft.drawLine(flakeX-5, flakeY, flakeX+5, flakeY, TFT_WHITE);
      tft.drawLine(flakeX, flakeY-5, flakeX, flakeY+5, TFT_WHITE);
    }
  } else if (weatherCode >= 95) {
    // Storm
    tft.fillCircle(x, y-20, 25, TFT_DARKGREY);
    tft.fillCircle(x+20, y-20, 20, TFT_DARKGREY);
    tft.fillCircle(x-15, y-15, 20, TFT_DARKGREY);
    // Lightning
    if ((int)(gradientOffset * 10) % 10 < 3) {
      tft.fillTriangle(x-5, y, x+10, y, x+5, y+15, TFT_YELLOW);
      tft.fillTriangle(x-10, y+15, x+5, y+15, x, y+30, TFT_YELLOW);
    }
  } else {
    // Cloudy
    tft.fillCircle(x, y, 25, TFT_LIGHTGREY);
    tft.fillCircle(x+20, y, 20, TFT_LIGHTGREY);
    tft.fillCircle(x-15, y+5, 20, TFT_LIGHTGREY);
  }
}

void fadeTransition() {
  for (int i = 0; i <= 5; i++) {
    int brightness = map(i, 0, 5, 0, 100);
    tft.fillScreen(tft.color565(brightness, brightness, brightness));
    delay(30);
  }
  tft.fillScreen(BACKGROUND_COLOR);
}

void drawScreenIndicator() {
  // Draw dots to show which screen we're on
  int dotY = tft.height() - INDICATOR_Y_OFFSET;
  int dotSpacing = 25;
  int startX = (tft.width() - (3 * dotSpacing - 10)) / 2;
  
  for (int i = 0; i < windows.size(); i++) {
    int dotX = startX + (i * dotSpacing);
    if (i == currentWindowIndex) {
      tft.fillCircle(dotX, dotY, 5, ACCENT_COLOR);
      // Add outer ring for better visibility
      tft.drawCircle(dotX, dotY, 6, ACCENT_COLOR);
    } else {
      tft.fillCircle(dotX, dotY, 4, TFT_DARKGREY);
    }
  }
}

void drawSmartMirrorWindow() {
  Serial.println("draw: smart mirror window");
  tft.fillScreen(BACKGROUND_COLOR);
  
  drawAnimatedBorder();
  
  // Get current time
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  
  // Time display - 12 hour format
  tft.setTextColor(SECONDARY_COLOR, BACKGROUND_COLOR);
  tft.setTextDatum(TC_DATUM);
  
  // Time in large font
  tft.setTextSize(6);
  String timeStr = get12HourTime(timeinfo.tm_hour, timeinfo.tm_min);
  
  // Split time and AM/PM for better display
  int spacePos = timeStr.indexOf(' ');
  String justTime = timeStr.substring(0, spacePos);
  String ampm = timeStr.substring(spacePos + 1);
  
  // Draw time
  tft.drawString(justTime, tft.width()/2, 40);
  
  // Draw AM/PM smaller
  tft.setTextSize(3);
  tft.drawString(ampm, tft.width()/2, 100);
  
  // Date
  tft.setTextColor(PRIMARY_COLOR, BACKGROUND_COLOR);
  char dateStr[30];
  sprintf(dateStr, "%s, %s %d", daysOfWeek[timeinfo.tm_wday], monthNames[timeinfo.tm_mon], timeinfo.tm_mday);
  tft.drawString(dateStr, tft.width()/2, 140);
  
  // Motivational quote
  tft.setTextSize(3);
  tft.setTextColor(VIBE_COLOR, BACKGROUND_COLOR);
  String quote = getMotivationalQuote();
  
  // Word wrap for quote
  int maxWidth = tft.width() - 40;
  int yPos = 200;
  
  if (tft.textWidth(quote) <= maxWidth) {
    tft.drawString(quote, tft.width()/2, yPos);
  } else {
    // Simple two-line split
    int spaceIdx = quote.lastIndexOf(' ', quote.length() / 2);
    if (spaceIdx > 0) {
      String line1 = quote.substring(0, spaceIdx);
      String line2 = quote.substring(spaceIdx + 1);
      tft.drawString(line1, tft.width()/2, yPos);
      tft.drawString(line2, tft.width()/2, yPos + 35);
    } else {
      tft.drawString(quote, tft.width()/2, yPos);
    }
  }
  
  drawScreenIndicator();
  tft.setTextDatum(TL_DATUM);
}

void drawWeatherWindow(){
  Serial.println("draw: weather window");
  tft.fillScreen(BACKGROUND_COLOR);
  
  drawAnimatedBorder();
  
  // Get actual weather data
  float currentTemp = 75.0;
  int humidity = 60;
  float windSpeed = 10.0;
  int weatherCode = 0;
  
  if (!weatherAPI.doc.isNull()) {
    currentTemp = weatherAPI.doc["current"]["temperature_2m"].as<float>();
    humidity = weatherAPI.doc["current"]["relative_humidity_2m"].as<int>();
    windSpeed = weatherAPI.doc["current"]["wind_speed_10m"].as<float>();
    weatherCode = weatherAPI.doc["current"]["weather_code"].as<int>();
  }

  // Weather description at top
  tft.setTextColor(VIBE_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(3);
  tft.setTextDatum(TC_DATUM);
  String desc = getWeatherDescription(currentTemp, weatherCode);
  tft.drawString(desc, tft.width()/2, 20);
  
  // Weather type text instead of emoji
  drawWeatherEmoji(weatherCode, tft.width()/2, 55);
  
  // Temperature - HUGE (without degree symbol)
  tft.setTextSize(10);
  tft.setTextColor(SECONDARY_COLOR, BACKGROUND_COLOR);
  String tempStr = String((int)currentTemp) + "F";
  tft.drawString(tempStr, tft.width()/2, 90);
  
  // Weather icon with animation
  drawWeatherIcon(tft.width()/2, WEATHER_ICON_Y, weatherCode);
  
  // Weather details - moved up to avoid overlap
  tft.setTextSize(3);
  tft.setTextDatum(TC_DATUM);
  
  // Humidity
  tft.setTextColor(PRIMARY_COLOR, BACKGROUND_COLOR);
  String humidStr = "Humidity: " + String(humidity) + "%";
  tft.drawString(humidStr, tft.width()/2, WEATHER_DETAILS_Y);
  
  // Wind - properly spaced
  String windStr = "Wind: " + String((int)windSpeed) + " mph";
  tft.drawString(windStr, tft.width()/2, WEATHER_DETAILS_Y + LINE_SPACING);
  
  drawScreenIndicator();
  tft.setTextDatum(TL_DATUM);
}

void drawJokeWindow(){
  Serial.println("draw: joke window");
  tft.fillScreen(BACKGROUND_COLOR);
  showingFullJoke = false;
  jokeTruncated = false;
  
  drawAnimatedBorder();
  
  // Title
  tft.setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(4);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("DAILY LAUGH", tft.width()/2, 20);
  
  // Joke container
  int jokeBoxY = 70;
  int jokeBoxHeight = 200;
  tft.fillRoundRect(15, jokeBoxY, tft.width()-30, jokeBoxHeight, 15, TFT_BLACK);
  tft.drawRoundRect(15, jokeBoxY, tft.width()-30, jokeBoxHeight, 15, SECONDARY_COLOR);
  tft.drawRoundRect(16, jokeBoxY+1, tft.width()-32, jokeBoxHeight-2, 15, SECONDARY_COLOR);
  
  // Get joke
  String joke = "Why don't scientists trust atoms? Because they make up everything!";
  if (!jokeAPI.doc["error"].as<bool>() && jokeAPI.doc.containsKey("joke")) {
    joke = jokeAPI.doc["joke"].as<String>();
  }
  currentFullJoke = joke;
  
  Serial.print("Joke length: ");
  Serial.println(joke.length());
  
  // Display joke with word wrap
  int xPos = 25;
  int yPos = jokeBoxY + 25;
  int maxWidth = tft.width() - 50;
  
  tft.setTextColor(PRIMARY_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(3);
  tft.setTextDatum(TL_DATUM);
  
  String words = joke + " ";
  String line = "";
  int spacePos = words.indexOf(' ');
  
  while (spacePos > -1 && !jokeTruncated) {
    String word = words.substring(0, spacePos + 1);
    words = words.substring(spacePos + 1);
    
    if (tft.textWidth(line + word) <= maxWidth) {
      line += word;
    } else {
      if (line.length() > 0) {
        tft.drawString(line, xPos, yPos);
        yPos += 35;
      }
      line = word;
      
      if (yPos > (jokeBoxY + jokeBoxHeight - 60)) {
        tft.drawString("...", xPos, yPos);
        jokeTruncated = true;
        
        // Show "Touch to read more" - simple text only
        tft.setTextSize(2);
        tft.setTextColor(VIBE_COLOR, BACKGROUND_COLOR);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("Touch to read more", tft.width()/2, jokeBoxY + jokeBoxHeight - 25);
        tft.setTextDatum(TL_DATUM);
        
        Serial.println("Joke truncated - showing touch to read more");
        break;
      }
    }
    
    spacePos = words.indexOf(' ');
    if (spacePos == -1 && words.length() > 0 && !jokeTruncated) {
      words += " ";
      spacePos = words.length() - 1;
    }
  }
  
  if (line.length() > 0 && !jokeTruncated) {
    tft.drawString(line, xPos, yPos);
  }
  
  // No emoji reactions - leave bottom area clean
  
  drawScreenIndicator();
  tft.setTextDatum(TL_DATUM);
}

void drawJokeFullWindow() {
  Serial.println("draw: full joke window");
  tft.fillScreen(BACKGROUND_COLOR);
  showingFullJoke = true;
  
  drawAnimatedBorder();
  
  // Title
  tft.setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(3);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("FULL JOKE", tft.width()/2, 20);
  
  // Display full joke with smaller text
  int xPos = 20;
  int yPos = 60;
  int maxWidth = tft.width() - 40;
  
  tft.setTextColor(PRIMARY_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  
  String words = currentFullJoke + " ";
  String line = "";
  int spacePos = words.indexOf(' ');
  
  while (spacePos > -1) {
    String word = words.substring(0, spacePos + 1);
    words = words.substring(spacePos + 1);
    
    if (tft.textWidth(line + word) <= maxWidth) {
      line += word;
    } else {
      if (line.length() > 0) {
        tft.drawString(line, xPos, yPos);
        yPos += 25;
        if (yPos > tft.height() - 60) break; // Safety check
      }
      line = word;
    }
    
    spacePos = words.indexOf(' ');
    if (spacePos == -1 && words.length() > 0) {
      words += " ";
      spacePos = words.length() - 1;
    }
  }
  
  if (line.length() > 0 && yPos <= tft.height() - 60) {
    tft.drawString(line, xPos, yPos);
  }
  
  // Touch to continue - simple text
  tft.setTextColor(VIBE_COLOR, BACKGROUND_COLOR);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("Touch to continue", tft.width()/2, tft.height() - 20);
  tft.setTextDatum(TL_DATUM);
}

void eventSmartMirrorWindow() {
  if (millis() - lastAnimationUpdate > ANIMATION_INTERVAL) {
    gradientOffset += 0.02;
    if (gradientOffset > 1.0) gradientOffset = 0.0;
    drawAnimatedBorder();
    lastAnimationUpdate = millis();
  }
  
  if (isTouched()) {
    lastTouch = millis();
    fadeTransition();
    switchToNextWindow();
  }
}

void eventWeatherWindow(){
  if (millis() - lastAnimationUpdate > ANIMATION_INTERVAL) {
    gradientOffset += 0.02;
    if (gradientOffset > 1.0) gradientOffset = 0.0;
    drawAnimatedBorder();
    
    // Redraw animated weather icon
    int weatherCode = 0;
    if (!weatherAPI.doc.isNull()) {
      weatherCode = weatherAPI.doc["current"]["weather_code"].as<int>();
    }
    drawWeatherIcon(tft.width()/2, WEATHER_ICON_Y, weatherCode);
    
    lastAnimationUpdate = millis();
  }
  
  if (isTouched()) {
    lastTouch = millis();
    fadeTransition();
    switchToNextWindow();
  }
}

void eventJokeWindow(){
  if (millis() - lastAnimationUpdate > ANIMATION_INTERVAL) {
    gradientOffset += 0.02;
    if (gradientOffset > 1.0) gradientOffset = 0.0;
    drawAnimatedBorder();
    lastAnimationUpdate = millis();
  }
  
  if (isTouched()) {
    lastTouch = millis();
    Serial.print("Touch detected. JokeTruncated: ");
    Serial.print(jokeTruncated);
    Serial.print(", ShowingFullJoke: ");
    Serial.println(showingFullJoke);
    
    if (jokeTruncated && !showingFullJoke) {
      // Show full joke
      Serial.println("Showing full joke");
      fadeTransition();
      drawJokeFullWindow();
    } else {
      // Go to next screen
      Serial.println("Going to next screen");
      fadeTransition();
      jokeAPI.updateData(); // Get new joke
      switchToNextWindow();
    }
  }
}

void eventJokeFullWindow() {
  if (millis() - lastAnimationUpdate > ANIMATION_INTERVAL) {
    gradientOffset += 0.02;
    if (gradientOffset > 1.0) gradientOffset = 0.0;
    drawAnimatedBorder();
    lastAnimationUpdate = millis();
  }
  
  if (isTouched()) {
    lastTouch = millis();
    Serial.println("Leaving full joke view");
    fadeTransition();
    showingFullJoke = false;
    jokeAPI.updateData(); // Get new joke
    switchToNextWindow();
  }
}

// ******DOG WINDOW*******
void drawDogWindow() {
  if (LittleFS.exists("/Dog.jpg") == true) {
    LittleFS.remove("/Dog.jpg");
  }

  Serial.println("draw: dog window");
  tft.fillScreen(BACKGROUND_COLOR);
  
  drawAnimatedBorder();
  
  // Title
  tft.setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(4);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("RANDOM DOG", tft.width()/2, 20);

 
  // Check if the image URL is available in the API response
  if (!dogAPI.doc["message"].isNull() && getFile(dogAPI.doc["message"].as<String>(), "/Dog.jpg")) {
    listLittleFS();
    TJpgDec.drawFsJpg(0, 0, "/Dog.jpg", LittleFS);
    // Check if the image is in JPEG or PNG format
    
  } else {
    // Fallback message if no image URL is available
    tft.setTextColor(PRIMARY_COLOR, BACKGROUND_COLOR);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("No dog image available", tft.width()/2, tft.height()/2);
  }
  
  drawScreenIndicator();
  tft.setTextDatum(TL_DATUM);
}

void eventDogWindow() {
  if (millis() - lastAnimationUpdate > ANIMATION_INTERVAL) {
    gradientOffset += 0.02;
    if (gradientOffset > 1.0) gradientOffset = 0.0;
    drawAnimatedBorder();
    lastAnimationUpdate = millis();
  }
  
  if (isTouched()) {
    lastTouch = millis();
    fadeTransition();
    //dogAPI.updateData(); // Get new dog image
    switchToNextWindow();
  }
}


void switchToWindow(int i){
  currentWindow = &windows.at(i % windows.size()); //handles out of bounds index
  lastScreenChange = millis();
  currentWindow->drawFunction();
}

void switchToNextWindow(){
  currentWindowIndex = (currentWindowIndex + 1) % windows.size();
  switchToWindow(currentWindowIndex);
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  //fs
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS initialisation failed!");
    //while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nLittleFS Initialisation done.");

  //touchscreen - PRESERVED EXACTLY AS IS
  tft.init();
  tft.setRotation(1);
  touchscreenSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, T_CS);
  touchscreen.begin(touchscreenSPI);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  //jpeg 
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // Cool splash screen
  tft.setTextSize(5);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("SMART", tft.width()/2, tft.height()/2 - 40);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("MIRROR", tft.width()/2, tft.height()/2 + 40);
  tft.setTextDatum(TL_DATUM);
  
  //windows
  windows.push_back(WindowObject(drawSmartMirrorWindow, eventSmartMirrorWindow));
  windows.push_back(WindowObject(drawWeatherWindow, eventWeatherWindow));
  windows.push_back(WindowObject(drawJokeWindow, eventJokeWindow));
  windows.push_back(WindowObject(drawDogWindow, eventDogWindow));

  //wifi init
  
  WiFiManager wm;

  // Check if reset button is pressed
  bool resetPressed = digitalRead(BOOT_PIN) == LOW;
  
  tft.fillRect(0, tft.height()-30, tft.width(), 30, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Connecting...", 10, tft.height()-25);

   // launch portal if 1) no creds, 2) reset button held 3 s, or 3) connect fails in 10 s
  //bool btnPressed = digitalRead(WIFI_RESET_PIN) == LOW;
  wm.setConfigPortalTimeout(180);          // auto-close in 3 min
  if (resetPressed || !wm.autoConnect("Mirror-Setup", "mirror123")) {
    wm.startConfigPortal("Mirror-Setup", "mirror123");
    tft.fillRect(0, tft.height()-60, tft.width(), 60, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Setup needed!", 10, tft.height()-55);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Join 'Mirror-Setup' on your phone", 10, tft.height()-35);
    tft.drawString("Password: mirror123", 10, tft.height()-20);
  }else{
    tft.fillRect(0, tft.height()-30, tft.width(), 30, TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Connected!", 10, tft.height()-25);
    
    // Configure NTP time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(1000);
  }
  /*
  // Reset settings if button pressed
  if (resetPressed) {
      tft.fillRect(0, tft.height()-30, tft.width(), 30, TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.drawString("Resetting WiFi...", 10, tft.height()-25);
      delay(1000);
      wm.resetSettings();
  }
  
  // Show connecting message
  tft.fillRect(0, tft.height()-30, tft.width(), 30, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Connecting...", 10, tft.height()-25);
  
  // Set AP callback to show portal message
  wm.setAPCallback([](WiFiManager *myWiFiManager) {
      tft.fillRect(0, tft.height()-60, tft.width(), 60, TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString("Setup needed!", 10, tft.height()-55);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("Join 'Mirror-Setup' on your phone", 10, tft.height()-35);
      tft.drawString("Password: mirror123", 10, tft.height()-20);
  });
  
  // Try to connect
  bool connected = wm.autoConnect("Mirror-Setup", "mirror123");
  
  if (connected) {
      tft.fillRect(0, tft.height()-30, tft.width(), 30, TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("Connected!", 10, tft.height()-25);
      
      // Configure NTP time
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      
      delay(1000);
  } else {
      tft.fillRect(0, tft.height()-30, tft.width(), 30, TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString("Connection Failed!", 10, tft.height()-25);
      delay(3000);
      ESP.restart();
  }
*/
  
  // Initialize APIs
  weatherAPI = APIObject(
    "https://api.open-meteo.com/v1/forecast?latitude=40.7146&longitude=-74.3646&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timezone=America%2FNew_York",
    60UL * 60UL * 1000UL  // 60 minute interval
  );

  jokeAPI = APIObject(
    "https://v2.jokeapi.dev/joke/Any?blacklistFlags=nsfw,religious,political,racist,sexist,explicit&type=single",
    5UL * 60UL * 1000UL  // 5 minute interval
  );

  dogAPI = APIObject(
    "https://dog.ceo/api/breeds/image/random",
    5UL * 60UL * 1000UL  // 5 minute interval
  );


  //REQUIRED first update to data
  weatherAPI.updateData();
  jokeAPI.updateData();
  dogAPI.updateData();

  // Seed random for quotes
  randomSeed(analogRead(0));

  //removes cached image
  if (LittleFS.exists("/Dog.jpg") == true) {
    LittleFS.remove("/Dog.jpg");
  }

  listLittleFS();

  //init window
  fadeTransition();
  switchToWindow(0);
}

void loop() {
  // API updates
  if(millis() - weatherAPI.lastUpdate >= weatherAPI.updateInterval || millis() < weatherAPI.lastUpdate){
    weatherAPI.updateData();
  }
  if(millis() - jokeAPI.lastUpdate >= jokeAPI.updateInterval || millis() < jokeAPI.lastUpdate){
    jokeAPI.updateData();
  }
  if(millis() - dogAPI.lastUpdate >= dogAPI.updateInterval || millis() < dogAPI.lastUpdate){
    dogAPI.updateData();
  }
  
  // Update time periodically (every minute)
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 60000 && currentWindowIndex == 0) {
    lastTimeUpdate = millis();
    drawSmartMirrorWindow(); // Redraw to update time
  }
  
  // Auto-rotation when no touch detected
  if (autoRotateEnabled && !showingFullJoke) {
    if (millis() - lastTouch > SCREEN_ROTATION_INTERVAL && 
        millis() - lastScreenChange > SCREEN_ROTATION_INTERVAL) {
      fadeTransition();
      switchToNextWindow();
    }
  }
  
  // Handle current window events
  if (showingFullJoke) {
    eventJokeFullWindow();
  } else {
    currentWindow->eventFunction();
  }
}
