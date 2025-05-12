#include <WiFi.h>
#include <HTTPClient.h>          // For HTTP over TCP
#include <WiFiClientSecure.h>    // For HTTPS
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson
#include <TFT_eSPI.h>            // Your display library

// ==== CONFIGURATION ====  
const char* SSID       = "Innovation_Foundry";
const char* PASSWORD   = "@Innovate22";

// Get a free API key from https://openweathermap.org/api
//https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m
const char* WEATHER_HOST = "api.open-meteo.com";
const char* WEATHER_PATH = "/v1/forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m";

// How often to refresh (ms)
const unsigned long UPDATE_INTERVAL = 60UL * 1000UL;  

TFT_eSPI tft = TFT_eSPI();
unsigned long lastUpdate = 0;

// ==== SETUP ====
void setup() {
  Serial.begin(115200);

  // 1) Wi-Fi connect
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println(" ✅");

  // 2) Init display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Initial message
  tft.drawString("Weather Demo", 10, 10, 4);
}

// ==== LOOP ====
void loop() {
  if (millis() - lastUpdate < UPDATE_INTERVAL) return;
  lastUpdate = millis();

  Serial.println("Doing update");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost – reconnecting");
    WiFi.reconnect();
    return;
  }

  // 3) Create secure client and HTTP object
  WiFiClientSecure client;
  client.setInsecure();  // skip certificate validation (for testing)

  HTTPClient https;
  String url = String("https://") + WEATHER_HOST + WEATHER_PATH;
  if (!https.begin(client, url)) {
    Serial.println("HTTPS begin failed");
    return;
  }

  // 4) GET request
  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    https.end();
    return;
  }

  // 5) Read payload
  String payload = https.getString();
  https.end();

  // 6) Parse JSON
  StaticJsonDocument<1024> doc;
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("JSON parse failed");
    return;
  }

  // 7) Extract data
  /*
  float temp    = doc["main"]["temp"];            // e.g. 18.7
  float feels   = doc["main"]["feels_like"];      // e.g. 17.2
  int   humidity= doc["main"]["humidity"];        // e.g. 82
  float windSpd = doc["wind"]["speed"];           // m/s
  const char* weatherDesc = doc["weather"][0]["main"]; // e.g. "Clouds"
*/
  float lat = doc["latitude"];
  Serial.print("Lat ");
  Serial.println(lat);
  /*
  Serial.printf("T:%.1f°C  Feels:%.1f°C  %s  Wind:%.1fm/s  Hum:%d%%\n",
                temp, feels, weatherDesc, windSpd, humidity);

  // 8) Update display
  // Clear a region
  tft.fillRect(0, 40, tft.width(), 100, TFT_BLACK);

  tft.setCursor(10, 40);
  tft.printf("Temp: %.1f C\n", temp);

  tft.setCursor(10, 60);
  tft.printf("Feels: %.1f C\n", feels);

  tft.setCursor(10, 80);
  tft.printf("Weather: %s\n", weatherDesc);

  tft.setCursor(10, 100);
  tft.printf("Wind: %.1f m/s\n", windSpd);

  tft.setCursor(10, 120);
  tft.printf("Humidity: %d%%\n", humidity);
  */
}

