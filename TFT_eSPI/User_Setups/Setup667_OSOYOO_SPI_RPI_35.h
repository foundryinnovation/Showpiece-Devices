// See SetupX_Template.h for all options available
#define USER_SETUP_ID 667

#define ILI9486_DRIVER
#define RPI_DISPLAY_TYPE


#define TFT_MISO 19    // SPI MISO (if reading/touch)
#define TFT_MOSI 23    // SPI MOSI
#define TFT_SCLK 18    // SPI clock
#define TFT_CS   15    // Chip select pin for TFT
#define TFT_DC   2     // Data/Command pin (LCD_RS)
#define TFT_RST  4     // Reset pin (or -1 if tied to ESP32 reset)

//#define TFT_BL 27
//#define TFT_BACKLIGHT_ON HIGH

// Optional touch screen chip select
//#define TOUCH_CS 5 // Chip select pin (T_CS) of touch screen

#define LOAD_GLCD    // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2   // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4   // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6   // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7   // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8   // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF   // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// TFT SPI clock frequency
// #define SPI_FREQUENCY  20000000
 #define SPI_FREQUENCY  27000000
//#define SPI_FREQUENCY  40000000
// #define SPI_FREQUENCY  80000000

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  20000000

// SPI clock frequency for touch controller
#define SPI_TOUCH_FREQUENCY  2500000
