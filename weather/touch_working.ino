#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

/*
These are the pins that need to be set for our touchscreen! (in User_Setup.h)
#define TFT_MOSI 13   // HSPI MOSI
#define TFT_MISO 12   // HSPI MISO (TFT SDO)
#define TFT_SCLK 14   // HSPI SCK
#define T_CS     33     // Touch CS
#define T_IRQ    36     // Touch IRQ (pen detect)

*/

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(T_CS, T_IRQ);

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);

  touchscreenSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, T_CS);
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
