#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

const int W =  480;
int16_t prevY[W];
int     phase = 0;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  for (int x = 0; x < W; x++) prevY[x] = tft.height()/2;
}

void loop() {
  tft.startWrite();  // batch SPI
  for (int x = 0; x < W; x++) {
    // Erase old pixel
    tft.drawPixel(x, prevY[x], TFT_BLACK);
    // Compute and draw new pixel
    int16_t y = sin(radians(x + phase)) * 50 + tft.height()/2;
    tft.drawPixel(x, y, TFT_GREEN);
    prevY[x] = y;
  }
  tft.endWrite();

  phase = (phase + 5) % 360;
  delay(30);
}

