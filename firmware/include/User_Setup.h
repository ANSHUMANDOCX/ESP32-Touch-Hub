#ifndef USER_SETUP_H
#define USER_SETUP_H

#define USER_SETUP_INFO "ESP32S3_ILI9488"

#define ILI9488_DRIVER

// Display size
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// SPI pins
#define TFT_MOSI 12
#define TFT_SCLK 13
#define TFT_MISO -1

#define TFT_CS   14
#define TFT_DC   11
#define TFT_RST  9

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// SPI speeds
#define SPI_FREQUENCY      27000000
#define SPI_READ_FREQUENCY 20000000

#endif