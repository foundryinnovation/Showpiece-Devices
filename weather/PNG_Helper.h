#include <pngle.h>


/*
include AFTER the line:
TFT_eSPI tft = TFT_eSPI();

!!!!
*/



void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4]);
void pngle_on_init(pngle_t *pngle, uint32_t w, uint32_t h);

pngle_t *pngle;

// Current drawing position
int16_t png_x = 0;
int16_t png_y = 0;

void drawPNG(const char* filename, int16_t x, int16_t y) {
  // Set drawing position
  png_x = x;
  png_y = y;
  
  // Open file
  fs::File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file");
    return;
  }
  
  // Create pngle instance
  pngle = pngle_new();
  if (!pngle) {
    Serial.println("Failed to create pngle");
    file.close();
    return;
  }
  
  // Set callbacks
  pngle_set_draw_callback(pngle, pngle_on_draw);
  
  // Optional: Set info callback to get image dimensions
  pngle_set_init_callback(pngle, pngle_on_init);
  
  // Feed data to pngle
  uint8_t buf[1024];
  int remain = 0;
  int len;
  
  tft.startWrite(); // Start SPI transaction for faster drawing
  
  while ((len = file.read(buf + remain, sizeof(buf) - remain)) > 0) {
    int fed = pngle_feed(pngle, buf, remain + len);
    if (fed < 0) {
      Serial.println("Error feeding data to pngle");
      break;
    }
    
    remain = remain + len - fed;
    if (remain > 0) memmove(buf, buf + fed, remain);
  }
  
  tft.endWrite(); // End SPI transaction
  
  // Cleanup
  pngle_destroy(pngle);
  file.close();
}

// Callback when image info is available
void pngle_on_init(pngle_t *pngle, uint32_t w, uint32_t h) {
  Serial.print("PNG Width: ");
  Serial.println(w);
  Serial.print("PNG Height: ");
  Serial.println(h);
}

// Callback for drawing pixels
void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4]) {
  // Extract RGB values
  uint8_t r = rgba[0];
  uint8_t g = rgba[1];
  uint8_t b = rgba[2];
  uint8_t a = rgba[3];
  
  // Only draw if pixel is not transparent
  if (a > 128) { // You can adjust this threshold
    // Convert to RGB565
    uint16_t color = tft.color565(r, g, b);
    
    // Draw pixel at the correct position
    tft.drawPixel(png_x + x, png_y + y, color);
  }
  // If a <= 128, pixel is transparent and we skip drawing it
}

// Alternative: If you want to handle semi-transparency with blending
void pngle_on_draw_with_blending(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4]) {
  uint8_t r = rgba[0];
  uint8_t g = rgba[1];
  uint8_t b = rgba[2];
  uint8_t a = rgba[3];
  
  if (a == 255) {
    // Fully opaque
    uint16_t color = tft.color565(r, g, b);
    tft.drawPixel(png_x + x, png_y + y, color);
  } else if (a > 0) {
    // Semi-transparent - blend with background
    uint16_t bgColor = tft.readPixel(png_x + x, png_y + y);
    
    // Extract background RGB
    uint8_t bg_r = (bgColor >> 11) << 3;
    uint8_t bg_g = ((bgColor >> 5) & 0x3F) << 2;
    uint8_t bg_b = (bgColor & 0x1F) << 3;
    
    // Alpha blend
    uint8_t inv_alpha = 255 - a;
    r = (r * a + bg_r * inv_alpha) / 255;
    g = (g * a + bg_g * inv_alpha) / 255;
    b = (b * a + bg_b * inv_alpha) / 255;
    
    uint16_t color = tft.color565(r, g, b);
    tft.drawPixel(png_x + x, png_y + y, color);
  }
  // If a == 0, pixel is fully transparent, don't draw
}

// Function to draw PNG with a specific transparent color replacement
void drawPNGWithBackground(const char* filename, int16_t x, int16_t y, uint16_t bgColor) {
  // First fill the area with background color
  // (You'd need to know the image dimensions for this)
  
  png_x = x;
  png_y = y;
  
  fs::File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file");
    return;
  }
  
  pngle = pngle_new();
  if (!pngle) {
    Serial.println("Failed to create pngle");
    file.close();
    return;
  }
  
  // Set draw callback that uses background color
  pngle_set_draw_callback(pngle, [](pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4]) {
    uint8_t a = rgba[3];
    if (a > 128) {
      uint16_t color = tft.color565(rgba[0], rgba[1], rgba[2]);
      tft.drawPixel(png_x + x, png_y + y, color);
    }
  });
  
  // Process the PNG
  uint8_t buf[1024];
  int remain = 0;
  int len;
  
  tft.startWrite();
  while ((len = file.read(buf + remain, sizeof(buf) - remain)) > 0) {
    int fed = pngle_feed(pngle, buf, remain + len);
    if (fed < 0) break;
    
    remain = remain + len - fed;
    if (remain > 0) memmove(buf, buf + fed, remain);
  }
  tft.endWrite();
  
  pngle_destroy(pngle);
  file.close();
}
