#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// Animation frame counter and timing
unsigned long animationTimer = 0;
const unsigned long ANIMATION_INTERVAL = 5000; // 5 seconds between animations
unsigned long jokeTimer = 0;
const unsigned long JOKE_INTERVAL = 10000; // 10 seconds for joke display
int animationFrame = 0;
bool isAnimating = false;
bool showingJoke = false;

// ==== CONFIGURATION ====
const char* SSID = "Innovation_Foundry";
const char* PASSWORD = "@Innovate22";

// Weather API Configuration - Summit, NJ (07901) coordinates with Fahrenheit
const char* WEATHER_HOST = "api.open-meteo.com";
// Updated for Summit, NJ coordinates and Fahrenheit units
const char* WEATHER_PATH = "/v1/forecast?latitude=40.7146&longitude=-74.3646&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timezone=America%2FNew_York";

// Joke API configuration
const char* JOKE_HOST = "v2.jokeapi.dev";
const char* JOKE_PATH = "/joke/Programming,Pun?blacklistFlags=nsfw,religious,political,racist,sexist,explicit&format=json&type=single";

// Update interval (every 30 minutes for weather)
const unsigned long UPDATE_INTERVAL = 30UL * 60UL * 1000UL;
// Update interval for jokes (every 5 minutes)
const unsigned long JOKE_UPDATE_INTERVAL = 5UL * 60UL * 1000UL;

// Screen configuration
TFT_eSPI tft = TFT_eSPI();
unsigned long lastWeatherUpdate = 0;
unsigned long lastJokeUpdate = 0;

// Define colors - more vibrant teen-friendly palette
#define BG_COLOR TFT_BLACK
#define TEXT_COLOR TFT_WHITE
#define HIGHLIGHT_COLOR TFT_YELLOW
#define TEMP_COLOR 0xFD20  // Bright pink/magenta
#define DESC_COLOR 0x07FF  // Bright cyan
#define ACCENT_COLOR 0x5D9B // Teal
#define ACCENT2_COLOR 0xFBE0 // Light pink
#define JOKE_BG_COLOR 0x3186 // Dark purple

// Display variables to track state
float lastTemp = -999.0;
float lastApparentTemp = -999.0;
int lastWeatherCode = -1;
int lastHumidity = -1;
float lastWindSpeed = -999.0;
String lastClothingRec = "";
float highTemp = 0.0;
float lowTemp = 0.0;
String currentJoke = "Why don't scientists trust atoms? Because they make up everything!";

// Display layout - adjusted for typical 3.5" landscape (width ~ 480, height ~ 320)
const int HEADER_Y = 10;
const int CURR_WEATHER_Y = 50;
const int ICON_SIZE = 120;  // Larger icon for visual impact
const int ICON_X = 20;
const int ICON_Y = CURR_WEATHER_Y;
const int TEMP_X = ICON_X + ICON_SIZE + 15;
const int TEMP_Y = ICON_Y + 20;
const int DESC_Y = ICON_Y + ICON_SIZE + 15;
const int CLOTHING_Y = DESC_Y + 60;
const int STATUS_Y = 280; // Moved near bottom for landscape (adjust as needed)
const int JOKE_Y = 80;    // Where the joke box will begin in landscape

// Forward declarations
void drawWeatherIcon(int x, int y, int size, int weatherCode);
String getWeatherDescription(int code);
String getClothingRecommendation(float maxTemp, float minTemp, float precipSum, int weatherCode);
void drawCenteredText(String text, int y, int fontSize);
void showError(String message);
String getTimeStr();
void updateDisplay(float currentTemp, float apparentTemp, int weatherCode, int humidity, float windSpeed, String clothingRec);
void updateTemperatureDisplay();
void updateClothingDisplay();
void performAnimation();
void pulseTemperature();
void scrollClothingRec();
void highlightWeatherIcon();
void showHighLowTemps();
void updateWeather();
void updateJoke();
void displayJoke(bool show);
void drawHeader();

// Helper function to create desired temp string with underscore if <100
String formatTemp(float temp) {
  int t = (int) round(temp);
  // If it's 100 or more, show all digits
  if (t >= 100) {
    return String(t);
  }
  // Otherwise, show an underscore for the hundreds digit and 2 digits for tens/ones
  // Example: 72 -> "_72", 7 -> "_07", 99 -> "_99"
  int absVal = abs(t);
  int tens = absVal / 10;
  int ones = absVal % 10;
  char buf[4];
  sprintf(buf, "_%d%d", tens, ones);
  // If the temperature could be negative, handle that here if needed
  // (Not likely if it's inside typical weather range, but just in case)
  if (t < 0) {
    return "-" + String(buf); // e.g., -5 -> "-_05"
  }
  return String(buf);
}

// Helper function to center text on screen
void drawCenteredText(String text, int y, int fontSize) {
    tft.setTextSize(fontSize);
    int textWidth = tft.textWidth(text);
    int x = (tft.width() - textWidth) / 2;
    tft.setCursor(x, y);
    tft.print(text);
}

void showError(String message) {
    // Display error at the bottom of the screen with larger text
    tft.fillRect(0, STATUS_Y, tft.width(), 25, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);  // Larger text for better visibility
    tft.drawString(message, 10, STATUS_Y, 2);
}

String getTimeStr() {
    // This is a placeholder - if you want actual time, you'll need to add
    // NTP client functionality or use a real-time clock module
    unsigned long uptime = millis() / 1000;
    int hours = (uptime / 3600) % 24;
    int minutes = (uptime / 60) % 60;
    
    String timeStr = "";
    if (hours < 10) timeStr += "0";
    timeStr += String(hours) + ":";
    if (minutes < 10) timeStr += "0";
    timeStr += String(minutes);
    
    return timeStr;
}

void drawHeader() {
    // Modern gradient-style header (40px tall)
    for (int i = 0; i < 40; i++) {
        uint16_t gradColor = tft.color565(0, 40 + i*2, 100 + i*5); // Gradient from dark to light blue
        tft.drawFastHLine(0, i, tft.width(), gradColor);
    }
    
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    
    // Draw teen-friendly header with small sun symbol
    int centerX = tft.width() / 2;
    tft.setCursor(centerX - 80, HEADER_Y);
    tft.print("SUMMIT");
    
    // Small weather symbol to the right
    tft.fillCircle(centerX + 85, HEADER_Y + 12, 10, TFT_YELLOW);
    tft.drawLine(centerX + 90, HEADER_Y + 2, centerX + 100, HEADER_Y + 10, TFT_YELLOW);
    tft.drawLine(centerX + 80, HEADER_Y + 12, centerX + 90, HEADER_Y + 20, TFT_YELLOW);
    
    tft.setTextColor(TEXT_COLOR, BG_COLOR);
}

String getWeatherDescription(int code) {
    switch(code) {
        case 0: return "Clear Sky";
        case 1: return "Mainly Clear";
        case 2: return "Partly Cloudy";
        case 3: return "Overcast";
        case 45: case 48: return "Foggy";
        case 51: return "Light Drizzle";
        case 53: return "Moderate Drizzle";
        case 55: return "Dense Drizzle";
        case 56: case 57: return "Freezing Drizzle";
        case 61: return "Light Rain";
        case 63: return "Moderate Rain";
        case 65: return "Heavy Rain";
        case 66: case 67: return "Freezing Rain";
        case 71: return "Light Snow";
        case 73: return "Moderate Snow";
        case 75: return "Heavy Snow";
        case 77: return "Snow Grains";
        case 80: return "Light Showers";
        case 81: return "Moderate Showers";
        case 82: return "Heavy Showers";
        case 85: return "Light Snow Showers";
        case 86: return "Heavy Snow Showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Thunderstorm & Hail";
        default: return "Unknown (" + String(code) + ")";
    }
}

String getClothingRecommendation(float maxTemp, float minTemp, float precipSum, int weatherCode) {
    String recommendation = "";
    
    // Temperature-based recommendations (Fahrenheit)
    if (maxTemp > 90) {
        recommendation += "Very hot! Light, breathable clothes.";
    } 
    else if (maxTemp > 80) {
        recommendation += "Hot. T-shirt and shorts ideal.";
    }
    else if (maxTemp > 70) {
        recommendation += "Warm. Light layers work well.";
    }
    else if (maxTemp > 60) {
        recommendation += "Mild. Light jacket or sweater.";
    }
    else if (maxTemp > 50) {
        recommendation += "Cool. Jacket needed.";
    }
    else if (maxTemp > 40) {
        recommendation += "Cold. Heavy jacket + layers.";
    }
    else if (maxTemp > 30) {
        recommendation += "Very cold! Winter coat & hat.";
    }
    else {
        recommendation += "Freezing! Full winter gear.";
    }
    
    // Precipitation advice
    if (precipSum > 0.2 || 
        (weatherCode >= 51 && weatherCode <= 67) || 
        (weatherCode >= 80 && weatherCode <= 82)) {
        recommendation += " Bring umbrella!";
    }
    else if ((weatherCode >= 71 && weatherCode <= 77) || (weatherCode >= 85 && weatherCode <= 86)) {
        recommendation += " Snow boots needed.";
    }
    
    if (maxTemp - minTemp > 20) {
        recommendation += " Dress in layers today.";
    }
    
    return recommendation;
}

// Main weather icon drawing function with simplified icons
void drawWeatherIcon(int x, int y, int size, int weatherCode) {
    // Simplified teen-friendly icons
    switch(weatherCode) {
        case 0: // Clear Sky (Sun)
        {
            tft.fillCircle(x + size/2, y + size/2, size/3, TFT_YELLOW);
            // Rays
            for (int i = 0; i < 8; i++) {
                float angle = i * PI / 4;
                int x1 = x + size/2 + cos(angle) * (size/3 + 5);
                int y1 = y + size/2 + sin(angle) * (size/3 + 5);
                int x2 = x + size/2 + cos(angle) * (size/2 + 10);
                int y2 = y + size/2 + sin(angle) * (size/2 + 10);
                // Thicker rays
                for (int j = -2; j <= 2; j++) {
                    int offsetX = sin(angle) * j;
                    int offsetY = -cos(angle) * j;
                    tft.drawLine(x1 + offsetX, y1 + offsetY, x2 + offsetX, y2 + offsetY, TFT_YELLOW);
                }
            }
        }
        break;
        
        case 1: // Mainly Clear
        case 2: // Partly Cloudy
        {
            // Smaller Sun
            tft.fillCircle(x + size/3, y + size/3, size/5, TFT_YELLOW);
            // Basic rays
            for (int i = 0; i < 4; i++) {
                float angle = i * PI / 4;
                int x1 = x + size/3 + cos(angle) * (size/5 + 2);
                int y1 = y + size/3 + sin(angle) * (size/5 + 2);
                int x2 = x + size/3 + cos(angle) * (size/4 + 5);
                int y2 = y + size/3 + sin(angle) * (size/4 + 5);
                tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
            }
            // Cloud
            tft.fillCircle(x + size*2/3, y + size*2/3, size/3, TFT_WHITE);
            tft.fillCircle(x + size/2, y + size*2/3, size/4, TFT_WHITE);
            tft.fillCircle(x + size*3/4, y + size/2, size/4, TFT_WHITE);
        }
        break;
        
        case 3: // Overcast
        {
            tft.fillRoundRect(x + size/6, y + size/3, size*2/3, size/3, size/8, 0xBDF7);
            tft.fillCircle(x + size/3, y + size/3, size/6, 0xBDF7);
            tft.fillCircle(x + size/2, y + size/4, size/5, 0xBDF7);
            tft.fillCircle(x + size*2/3, y + size/3, size/6, 0xBDF7);
        }
        break;
        
        case 51: case 53: case 55: // Drizzle
        case 61: case 63: case 65: // Rain
        case 80: case 81: case 82: // Showers
        {
            // Cloud
            tft.fillRoundRect(x + size/6, y + size/5, size*2/3, size/3, size/8, 0x7BEF);
            tft.fillCircle(x + size/3, y + size/5, size/6, 0x7BEF);
            tft.fillCircle(x + size/2, y + size/6, size/5, 0x7BEF);
            tft.fillCircle(x + size*2/3, y + size/5, size/6, 0x7BEF);
            // Raindrops
            for (int i = 0; i < 5; i++) {
                int xOffset = random(-5, 5);
                int yOffset = random(-3, 3);
                int dropX = x + size/5 + i*size/5 + xOffset;
                int dropY = y + size*2/3 + yOffset;
                tft.fillRoundRect(dropX - 2, dropY, 4, size/5, 2, 0x05FF);
            }
        }
        break;
        
        case 71: case 73: case 75: case 77: // Snow
        case 85: case 86: // Snow showers
        {
            // Cloud
            tft.fillRoundRect(x + size/6, y + size/5, size*2/3, size/3, size/8, 0x7BEF);
            tft.fillCircle(x + size/3, y + size/5, size/6, 0x7BEF);
            tft.fillCircle(x + size/2, y + size/6, size/5, 0x7BEF);
            tft.fillCircle(x + size*2/3, y + size/5, size/6, 0x7BEF);
            // Snowflakes
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 2; j++) {
                    int flakeX = x + size/5 + i*size/5;
                    int flakeY = y + size/2 + j*size/5;
                    tft.fillCircle(flakeX, flakeY, 3, TFT_WHITE);
                    tft.drawLine(flakeX - 5, flakeY, flakeX + 5, flakeY, TFT_WHITE);
                    tft.drawLine(flakeX, flakeY - 5, flakeX, flakeY + 5, TFT_WHITE);
                    tft.drawLine(flakeX - 3, flakeY - 3, flakeX + 3, flakeY + 3, TFT_WHITE);
                    tft.drawLine(flakeX + 3, flakeY - 3, flakeX - 3, flakeY + 3, TFT_WHITE);
                }
            }
        }
        break;
        
        default:
        {
            // Fallback for unknown codes
            // For precipitation, or other codes, show a question mark
            tft.setTextColor(TFT_YELLOW);
            tft.setTextSize(6);
            tft.setCursor(x + size/2 - 15, y + size/2 - 20);
            tft.print("?");
            // Small code display
            tft.setTextSize(2);
            tft.setCursor(x + size/2 - 15, y + size*3/4);
            tft.print(weatherCode);
        }
        break;
    }
}

void updateTemperatureDisplay() {
    // Clear temperature area
    int centerX = tft.width() / 2;
    int tempBoxWidth = 220;
    int tempBoxHeight = 80;
    int tempBoxX = centerX - tempBoxWidth / 2;
    int tempBoxY = DESC_Y - 20;
    tft.fillRect(tempBoxX, tempBoxY, tempBoxWidth, tempBoxHeight, BG_COLOR);

    // Convert temps with new format
    String mainTempStr = formatTemp(lastTemp) + "°F";
    String feelsStr = "FEELS LIKE: " + formatTemp(lastApparentTemp) + "°F";

    // Draw main temperature
    tft.setTextColor(TEMP_COLOR, BG_COLOR);
    tft.setTextSize(5);
    int tempWidth = tft.textWidth(mainTempStr);
    tft.setCursor(centerX - tempWidth / 2, tempBoxY + 15);
    tft.print(mainTempStr);

    // Display "feels like"
    tft.setTextColor(ACCENT2_COLOR, BG_COLOR);
    tft.setTextSize(2);
    int feelsWidth = tft.textWidth(feelsStr);
    tft.setCursor(centerX - feelsWidth / 2, tempBoxY + 60);
    tft.print(feelsStr);
}

void updateClothingDisplay() {
    // Clear previous recommendation area
    tft.fillRect(0, CLOTHING_Y, tft.width(), 70, BG_COLOR);
    
    // Create a modern "card" effect
    int cardWidth = tft.width() - 20;
    int cardHeight = 60;
    int cardX = 10;
    int cardY = CLOTHING_Y;
    
    // Gradient background
    for (int i = 0; i < cardHeight; i++) {
        uint16_t gradColor = tft.color565(40 - i/3, 0, 80 - i/3);
        tft.drawFastHLine(cardX, cardY + i, cardWidth, gradColor);
    }
    tft.drawRoundRect(cardX, cardY, cardWidth, cardHeight, 8, ACCENT_COLOR);
    
    // Style icon
    tft.fillCircle(cardX + 20, cardY + 20, 10, ACCENT2_COLOR);
    tft.fillRect(cardX + 15, cardY + 20, 10, 25, ACCENT2_COLOR);
    
    // Text
    tft.setTextColor(TFT_WHITE, 0x0000);
    tft.setTextSize(2);
    String visibleText = lastClothingRec;
    if (visibleText.length() > 35) {
        visibleText = visibleText.substring(0, 32) + "...";
    }
    
    tft.setCursor(cardX + 40, cardY + 20);
    tft.print(visibleText);
}

void updateDisplay(float currentTemp, float apparentTemp, int weatherCode, 
                  int humidity, float windSpeed, String clothingRec) {
    
    tft.startWrite(); // Batch drawing for smoother updates
    
    // Clear main display area
    tft.fillRect(0, CURR_WEATHER_Y, tft.width(), CLOTHING_Y + 80, BG_COLOR);

    // Draw weather icon in center horizontally
    int iconCenterX = tft.width() / 2 - ICON_SIZE / 2;
    drawWeatherIcon(iconCenterX, ICON_Y, ICON_SIZE, weatherCode);
    lastWeatherCode = weatherCode;

    // Weather description
    tft.fillRect(0, DESC_Y, tft.width(), 35, BG_COLOR);
    tft.setTextColor(DESC_COLOR, BG_COLOR);
    tft.setTextSize(2);
    drawCenteredText(getWeatherDescription(weatherCode), DESC_Y, 2);
    
    // Store updated temps
    lastTemp = currentTemp;
    lastApparentTemp = apparentTemp;
    // Display temps
    updateTemperatureDisplay();
    
    // Show high/low temps visually
    tft.fillRect(0, DESC_Y + 45, tft.width(), 30, BG_COLOR);
    int barWidth = tft.width() - 60;
    int barHeight = 15;
    int barY = DESC_Y + 50;
    int barX = 30;
    tft.fillRoundRect(barX, barY, barWidth, barHeight, 8, TFT_DARKGREY);
    int rangeSpan = highTemp - lowTemp;
    if (rangeSpan < 5) rangeSpan = 5;
    float tempStep = (float)barWidth / rangeSpan;
    for (int i = 0; i < rangeSpan; i++) {
        uint8_t red = min(255, (i * 255) / rangeSpan);
        uint8_t blue = min(255, 255 - ((i * 255) / rangeSpan));
        uint16_t color = tft.color565(red, 0, blue);
        tft.drawFastVLine(barX + i * tempStep, barY, barHeight, color);
    }
    // Low label
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setCursor(barX - 25, barY + 2);
    tft.printf("%.0f", lowTemp);
    // High label
    tft.setCursor(barX + barWidth + 8, barY + 2);
    tft.printf("%.0f", highTemp);
    
    // Update clothing recommendation
    lastClothingRec = clothingRec;
    updateClothingDisplay();
    
    lastHumidity = humidity;
    lastWindSpeed = windSpeed;
    
    tft.endWrite();
}

void pulseTemperature() {
    // Simple pulse effect on the main temperature
    float originalSize = 5.0;
    float pulseSize = 6.0;  
    String tempStr = formatTemp(lastTemp) + "°F";

    // Coordinates for the main temp
    int centerX = tft.width() / 2;
    int tempBoxY = DESC_Y - 20;
    int yBase = tempBoxY + 15;
    
    for (int i = 0; i < 3; i++) {
        // Pulse color
        uint16_t pulseColor = (i % 2 == 0) ? TEMP_COLOR : ACCENT2_COLOR;
        
        // Erase old
        tft.fillRect(0, tempBoxY, tft.width(), 80, BG_COLOR);
        
        // Pulse bigger
        tft.setTextColor(pulseColor, BG_COLOR);
        tft.setTextSize(pulseSize);
        int tempWidth = tft.textWidth(tempStr);
        tft.setCursor(centerX - tempWidth / 2, yBase);
        tft.print(tempStr);
        delay(300);
        
        // Return to normal
        tft.fillRect(0, tempBoxY, tft.width(), 80, BG_COLOR);
        tft.setTextColor(TEMP_COLOR, BG_COLOR);
        tft.setTextSize(originalSize);
        tempWidth = tft.textWidth(tempStr);
        tft.setCursor(centerX - tempWidth / 2, yBase);
        tft.print(tempStr);
        delay(300);
    }
    
    // Finally restore the entire area properly
    updateTemperatureDisplay();
}

void scrollClothingRec() {
    // Scroll the clothing recommendation if it's long
    if (lastClothingRec.length() < 30) return;
    
    int cardX = 10;
    int cardY = CLOTHING_Y;
    int cardWidth = tft.width() - 20;
    int cardHeight = 60;
    
    String scrollText = "~ " + lastClothingRec + " ~ " + lastClothingRec.substring(0, 20);
    
    for (int pos = 0; pos < lastClothingRec.length() + 5; pos += 3) {
        tft.fillRect(cardX + 40, cardY + 15, cardWidth - 50, 30, 0x0019);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        
        String visibleText = scrollText.substring(pos, min(pos + 40, (int)scrollText.length()));
        tft.setCursor(cardX + 40, cardY + 20);
        tft.print(visibleText);
        
        delay(200);
    }
    
    updateClothingDisplay();
}

void highlightWeatherIcon() {
    // "Ripple" effect
    int iconCenterX = tft.width() / 2;
    int centerY = ICON_Y + ICON_SIZE/2;
    for (int size = 5; size <= 20; size += 5) {
        tft.drawCircle(iconCenterX, centerY, ICON_SIZE/2 + size, ACCENT2_COLOR);
        delay(150);
    }
    // Flash icon
    tft.fillRect(iconCenterX - ICON_SIZE/2, ICON_Y, ICON_SIZE, ICON_SIZE, BG_COLOR);
    drawWeatherIcon(iconCenterX - ICON_SIZE/2, ICON_Y, ICON_SIZE, lastWeatherCode);
    delay(200);
    tft.invertDisplay(true);
    delay(100);
    tft.invertDisplay(false);
    delay(100);
    tft.fillRect(iconCenterX - ICON_SIZE/2, ICON_Y, ICON_SIZE, ICON_SIZE, BG_COLOR);
    drawWeatherIcon(iconCenterX - ICON_SIZE/2, ICON_Y, ICON_SIZE, lastWeatherCode);
    
    // Clear ripples
    for (int size = 20; size >= 5; size -= 5) {
        tft.drawCircle(iconCenterX, centerY, ICON_SIZE/2 + size, BG_COLOR);
        delay(100);
    }
}

void showHighLowTemps() {
    // Flash the temp bar area
    for (int i = 0; i < 3; i++) {
        tft.fillRect(0, DESC_Y + 35, tft.width(), 25, ACCENT_COLOR);
        delay(150);
        tft.fillRect(0, DESC_Y + 35, tft.width(), 25, BG_COLOR);
        delay(150);
    }
    
    // Animate a fill bar
    int barWidth = tft.width() - 40;
    int barHeight = 20;
    int barY = DESC_Y + 40;
    int barX = 20;
    tft.fillRoundRect(barX, barY, barWidth, barHeight, 10, TFT_DARKGREY);
    for (int i = 0; i <= barWidth; i += 10) {
        for (int j = 0; j < i; j++) {
            float ratio = (float)j / barWidth;
            uint8_t r = 255 * ratio;
            uint8_t b = 255 * (1 - ratio);
            uint16_t color = tft.color565(r, 20, b);
            tft.drawFastVLine(barX + j, barY, barHeight, color);
        }
        delay(50);
    }
    
    // Markers
    tft.setTextSize(3);
    // Low
    tft.fillCircle(barX - 5, barY + barHeight/2, 15, 0x001F); 
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(barX - 15, barY + barHeight/2 - 10);
    tft.printf("%.0f", lowTemp);
    // High
    tft.fillCircle(barX + barWidth + 5, barY + barHeight/2, 15, 0xF800);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(barX + barWidth - 5, barY + barHeight/2 - 10);
    tft.printf("%.0f", highTemp);
    
    delay(3000);
    tft.fillRect(0, DESC_Y + 35, tft.width(), 30, BG_COLOR);
    
    updateDisplay(lastTemp, lastApparentTemp, lastWeatherCode, lastHumidity, lastWindSpeed, lastClothingRec);
}

void updateWeather() {
    Serial.println("Updating weather data...");
    // Show updating status
    tft.fillRect(0, STATUS_Y, tft.width(), 25, BG_COLOR);
    tft.setTextColor(TFT_DARKGREY, BG_COLOR);
    tft.drawString("Updating weather...", 10, STATUS_Y, 2);
    
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected - reconnecting");
        tft.fillRect(0, STATUS_Y, tft.width(), 25, BG_COLOR);
        tft.drawString("WiFi Disconnected. Reconnecting...", 10, STATUS_Y, 2);
        
        WiFi.reconnect();
        delay(5000);
        
        if (WiFi.status() != WL_CONNECTED) {
            tft.drawString("WiFi Failed. Will retry later.", 10, STATUS_Y, 2);
            return;
        }
    }
    
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    String url = String("https://") + WEATHER_HOST + WEATHER_PATH;
    Serial.print("Requesting URL: "); Serial.println(url);
    
    if (!https.begin(client, url)) {
        Serial.println("HTTPS begin failed");
        showError("Connection Failed");
        return;
    }
    
    int httpCode = https.GET();
    if (httpCode <= 0) {
        Serial.printf("HTTP GET failed, error: %s\n", https.errorToString(httpCode).c_str());
        showError("HTTP Error: " + https.errorToString(httpCode));
        https.end();
        return;
    }
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        https.end();
        
        DynamicJsonDocument doc(6144);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            showError("JSON Parse Error");
            return;
        }
        
        float currentTemp = doc["current"]["temperature_2m"];
        float apparentTemp = doc["current"]["apparent_temperature"];
        int weatherCode = doc["current"]["weather_code"];
        int humidity = doc["current"]["relative_humidity_2m"];
        float windSpeed = doc["current"]["wind_speed_10m"];
        
        float maxTemp = doc["daily"]["temperature_2m_max"][0];
        float minTemp = doc["daily"]["temperature_2m_min"][0];
        float precipSum = doc["daily"]["precipitation_sum"][0];
        
        highTemp = maxTemp;
        lowTemp = minTemp;
        
        Serial.printf("Current: %.1f°F (Feels: %.1f°F), Code: %d, Humidity: %d%%, Wind: %.1f mph\n", 
                     currentTemp, apparentTemp, weatherCode, humidity, windSpeed);
        Serial.printf("Today: High: %.1f°F, Low: %.1f°F, Precip: %.2f in\n", maxTemp, minTemp, precipSum);
        
        String clothingRec = getClothingRecommendation(maxTemp, minTemp, precipSum, weatherCode);
        
        // Update display only if not showing joke
        if (!showingJoke) {
            updateDisplay(currentTemp, apparentTemp, weatherCode, humidity, windSpeed, clothingRec);
        }
        
        // Last updated time
        tft.fillRect(0, STATUS_Y, tft.width(), 25, BG_COLOR);
        tft.setTextColor(TFT_DARKGREY, BG_COLOR);
        String updateMsg = "Updated: " + getTimeStr();
        int textWidth = tft.textWidth(updateMsg);
        int centerX = (tft.width() - textWidth) / 2;
        tft.drawString(updateMsg, centerX, STATUS_Y, 2);
        
    } else {
        Serial.printf("HTTP GET failed, code: %d\n", httpCode);
        showError("HTTP Error: " + String(httpCode));
        https.end();
    }
}

void updateJoke() {
    Serial.println("Updating joke...");
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected - cannot update joke");
        return;
    }
    
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    String url = String("https://") + JOKE_HOST + JOKE_PATH;
    Serial.print("Requesting joke URL: "); Serial.println(url);
    
    if (!https.begin(client, url)) {
        Serial.println("HTTPS begin failed for joke");
        return;
    }
    
    int httpCode = https.GET();
    if (httpCode <= 0) {
        Serial.printf("HTTP GET failed for joke, error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        return;
    }
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        https.end();
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("deserializeJson() failed for joke: ");
            Serial.println(error.c_str());
            return;
        }
        
        if (doc["error"] == false) {
            currentJoke = doc["joke"].as<String>();
            Serial.println("New joke: " + currentJoke);
        }
    } else {
        Serial.printf("HTTP GET failed for joke, code: %d\n", httpCode);
        https.end();
    }
}

void displayJoke(bool show) {
    if (show) {
        // Hide everything else by filling the entire screen
        tft.fillScreen(JOKE_BG_COLOR);
        
        // Draw a nice container for the joke
        int jokeBorderRadius = 15;
        tft.fillRoundRect(10, JOKE_Y, tft.width() - 20, 120, jokeBorderRadius, 0x4A69);
        tft.drawRoundRect(10, JOKE_Y, tft.width() - 20, 120, jokeBorderRadius, 0x7BEF);
        
        // Emoji
        tft.fillCircle(35, JOKE_Y + 30, 15, TFT_YELLOW);
        tft.fillCircle(28, JOKE_Y + 25, 3, TFT_BLACK); // Left eye
        tft.fillCircle(42, JOKE_Y + 25, 3, TFT_BLACK); // Right eye
        // Simple "smile" arc:
        tft.drawArc(35, JOKE_Y + 30, 10, 8, 45, 135, TFT_BLACK, TFT_YELLOW);
        
        // Joke title
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.setCursor(60, JOKE_Y + 20);
        tft.print("JOKE OF THE DAY:");
        
        // Word-wrap the joke
        int xPos = 20;
        int yPos = JOKE_Y + 50;
        int maxWidth = tft.width() - 40;
        
        String words = currentJoke + " ";
        String line = "";
        int spacePos = words.indexOf(' ');
        
        while (spacePos > -1) {
            String word = words.substring(0, spacePos + 1);
            words = words.substring(spacePos + 1);
            if (tft.textWidth(line + word) <= maxWidth) {
                line += word;
            } else {
                tft.setCursor(xPos, yPos);
                tft.print(line);
                line = word;
                yPos += 22; 
                if (yPos > (JOKE_Y + 110)) {
                    tft.setCursor(xPos, yPos);
                    tft.print(line + "...");
                    break;
                }
            }
            spacePos = words.indexOf(' ');
            if (spacePos == -1 && words.length() > 0) {
                words += " ";
                spacePos = words.length() - 1;
            }
        }
        if (line.length() > 0 && yPos <= (JOKE_Y + 110)) {
            tft.setCursor(xPos, yPos);
            tft.print(line);
        }
    } 
    else {
        // Restore full weather display
        tft.fillScreen(BG_COLOR);
        drawHeader();
        updateDisplay(lastTemp, lastApparentTemp, lastWeatherCode, lastHumidity, lastWindSpeed, lastClothingRec);
    }
}

void performAnimation() {
    if (isAnimating) return;
    isAnimating = true;
    
    switch (animationFrame % 4) {
        case 0:
            pulseTemperature();
            break;
        case 1:
            scrollClothingRec();
            break;
        case 2:
            highlightWeatherIcon();
            break;
        case 3:
            showHighLowTemps();
            break;
    }
    
    animationFrame++;
    isAnimating = false;
}

void setup() {
    Serial.begin(115200);
    
    tft.init();
    // Use rotation(1) for landscape (common on many 3.5" TFT displays)
    tft.setRotation(1);
    tft.fillScreen(BG_COLOR);
    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    
    // Startup message
    drawCenteredText("SUMMIT WEATHER", HEADER_Y, 3);
    drawCenteredText("Connecting to WiFi...", CURR_WEATHER_Y, 2);
    
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to WiFi");
    
    int dotCount = 0;
    while (WiFi.status() != WL_CONNECTED && dotCount < 20) {
        delay(500);
        Serial.print('.');
        tft.drawChar('.', (tft.width()/2) + (dotCount%3)*15, CURR_WEATHER_Y+30, 2);
        dotCount++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        tft.fillRect(0, CURR_WEATHER_Y, tft.width(), 40, BG_COLOR);
        drawCenteredText("Connected!", CURR_WEATHER_Y, 2);
        delay(1000);
    } else {
        Serial.println(" Failed to connect!");
        drawCenteredText("WiFi Failed. Retrying...", CURR_WEATHER_Y+30, 2);
        delay(3000);
        ESP.restart();
    }
    
    // Clear screen and show header
    tft.fillScreen(BG_COLOR);
    drawHeader();
    
    // Initial updates
    updateWeather();
    updateJoke();
    
    lastWeatherUpdate = millis();
    lastJokeUpdate = millis();
    animationTimer = millis();
    jokeTimer = millis() + JOKE_INTERVAL; // Start with weather, show joke after interval
}

void loop() {
    // Check weather updates
    if (millis() - lastWeatherUpdate >= UPDATE_INTERVAL || millis() < lastWeatherUpdate) {
        updateWeather();
        lastWeatherUpdate = millis();
    }
    
    // Check joke updates
    if (millis() - lastJokeUpdate >= JOKE_UPDATE_INTERVAL || millis() < lastJokeUpdate) {
        updateJoke();
        lastJokeUpdate = millis();
    }
    
    // Toggle between joke and weather every JOKE_INTERVAL
    if (millis() - jokeTimer >= JOKE_INTERVAL) {
        showingJoke = !showingJoke;
        displayJoke(showingJoke);
        jokeTimer = millis();
    }
    
    // Animate if not showing joke
    if (!showingJoke && millis() - animationTimer >= ANIMATION_INTERVAL) {
        performAnimation();
        animationTimer = millis();
    }
    
    delay(100);
}

