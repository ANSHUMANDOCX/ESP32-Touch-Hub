#pragma once

// ─── Display ───────────────────────────
#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST   14
#define TFT_MOSI  11
#define TFT_MISO  13
#define TFT_SCK   12
#define TFT_LED   47   // backlight PWM
#define TOUCH_CS  15
#define TOUCH_IRQ 16

// ─── SD Card (TF1 - dedicated SPI) ─────
#define SD_CS     21
#define SD_MOSI   40
#define SD_MISO   41
#define SD_SCK    42
#define SD_CD      1

// ─── Buttons ───────────────────────────
#define BTN_BACK    4
#define BTN_HOME    5
#define BTN_CUSTOM  6

// ─── Buzzer ────────────────────────────
#define BUZZER_PIN 48

// ─── Battery ───────────────────────────
#define BATT_ADC    3
#define STAT1_PIN  17
#define STAT2_PIN  16
#define PG_PIN      5

// ─── WiFi ──────────────────────────────
#define WIFI_SSID     "YOUR_SSID"
#define WIFI_PASS     "YOUR_PASSWORD"
#define NTP_SERVER    "pool.ntp.org"
#define NTP_OFFSET    19800   // UTC+5:30 India — change to your timezone

// ─── LVGL Buffer ───────────────────────
#define LVGL_BUF_SIZE (480 * 40)

// ─── Battery thresholds ────────────────
#define BATT_MIN_MV  3000
#define BATT_MAX_MV  4200
#define BATT_LOW_PCT   15
