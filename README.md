# Getting screen to work
1. Go to the TFT\_eSPI folder (Arduino/libraries/TFT\_eSPI)
2. Edit the 'User\_Setup\_Select.h' and make sure the only include is '#include <User_Setup.h>'
3. Edit the 'User_Setup.h' and make sure the '#define ILI9486_DRIVER' is uncommented
4. Edit the pins that are connected to the arduino; example:
```
#define TFT_CS   15    // Chip select pin for TFT 
#define TFT_DC   2     // Data/Command pin (LCD_RS)
#define TFT_RST  4     // Reset pin (or -1 if tied to ESP32 reset)
#define TFT_MOSI 23    // SPI MOSI
#define TFT_MISO 19    // SPI MISO (if reading/touch)
#define TFT_SCLK 18    // SPI clock
```
