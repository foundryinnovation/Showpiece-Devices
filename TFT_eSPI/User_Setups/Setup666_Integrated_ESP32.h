// See SetupX_Template.h for all options available
#define USER_SETUP_ID 666

#define USER_SETUP_INFO "ESP32_3248S035R_ST7796_XPT2046"
#define USE_HSPI_PORT

#define ILI9488_DRIVER 



#define TFT_MISO 12 // (leave TFT SDO disconnected if other SPI devices share MISO)
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15  // Chip select control pin
#define TFT_DC 2   // Data Command control pin
#define TFT_RST -1 // Reset pin (could connect to RST pin)
#define TFT_BL 27

#define TFT_BACKLIGHT_ON HIGH

// Touch controller (XPT2046) on the same HSPI bus:
#define XPT2046_TOUCH    // enable the XPT2046 resistive touch driver
#define T_CS     33     // Touch CS
#define T_IRQ    36     // Touch IRQ (pen detect)

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
// #define SPI_FREQUENCY  27000000
#define SPI_FREQUENCY  40000000
// #define SPI_FREQUENCY  80000000

// Optional reduced SPI frequency for reading TFT
//#define SPI_READ_FREQUENCY  16000000
#define SPI_READ_FREQUENCY  20000000

// SPI clock frequency for touch controller
#define SPI_TOUCH_FREQUENCY  2500000

#define SUPPORT_TRANSACTIONS
