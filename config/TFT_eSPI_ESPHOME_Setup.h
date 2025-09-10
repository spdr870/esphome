// --- TFT_eSPI setup for ESPHome (ILI9488 480x320, landscape) ---
#define USER_SETUP_ID 1001

// Driver
#define ILI9488_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
#define TFT_RGB_ORDER TFT_BGR  // veel ILI9488 modules gebruiken BGR

// SPI pins
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   21
#define TFT_RST  -1   // geen reset pin

// Touch (wordt door XPT2046 lib gebruikt, hier alleen ter referentie)
#define TOUCH_CS  26

// Speeds
#define SPI_FREQUENCY  40000000
//#define SPI_READ_FREQUENCY 20000000  // indien nodig

// Backlight (optioneel, alleen als je BL pin hebt en macro gebruikt in code)
#define TFT_BACKLIGHT_ON 1
