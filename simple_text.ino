#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // TFT instance

void setup() {
  tft.init();                // Initialize TFT
  tft.setRotation(1);        // Set rotation (0-3) as needed for orientation
  tft.fillScreen(TFT_BLUE); // Clear screen to black
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString("david is very suave", 50, 100, 4);
}

void loop() {
  // You can add touch reading or updates here
}
