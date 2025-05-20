#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 13  // T_DIN
#define XPT2046_MISO 12  // T_OUT
#define XPT2046_CLK 14   // T_CLK
#define XPT2046_CS 33    // T_CS


SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
}

void loop() {
  
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    Serial.print("Touched ");
    TS_Point p = touchscreen.getPoint();
    p.x = map(p.x, 200, 3700, 1, 320);
    p.y = map(p.y, 240, 3800, 1, 240);

    Serial.print(p.x);
    Serial.print(" ");
    Serial.print(p.y);
    Serial.println();
    tft.fillCircle(p.x,p.y,2,0xFF0000);
  }
}
