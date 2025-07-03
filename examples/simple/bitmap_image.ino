#include <TFT_eSPI.h>
#include <SPI.h>

// original 8×8 invader
static const uint8_t INV8_W = 8, INV8_H = 8;
const uint8_t PROGMEM inv8[INV8_H] = {
  0b00111100,
  0b01111110,
  0b11011011,
  0b11111111,
  0b11111111,
  0b10111101,
  0b00100100,
  0b01011010
};

TFT_eSPI tft = TFT_eSPI();

// draw the 8×8 bitmap, scaling each pixel to a scale×scale block
void drawInvader128(int16_t x0, int16_t y0, uint16_t color) {
  const uint8_t scale = 16;            // 16×16 blocks → 8×16 = 128
  for (uint8_t row = 0; row < INV8_H; row++) {
    uint8_t bits = pgm_read_byte(&inv8[row]);
    for (uint8_t col = 0; col < INV8_W; col++) {
      if (bits & (0x80 >> col)) {
        // draw a filled block for each “on” bit
        tft.fillRect(
          x0 + col * scale,
          y0 + row * scale,
          scale, scale,
          color
        );
      }
    }
  }
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // center the 128×128 invader on a 240×320 display
  int16_t x = (tft.width()  - INV8_W*16) / 2;
  int16_t y = (tft.height() - INV8_H*16) / 2;

  drawInvader128(x, y, TFT_GREEN);
}

void loop() {
  // nothing here
}

