/*
 * ============================================================
 *  ESP32-S3 Smart Hub — Concept C Dashboard
 * ============================================================
 *  Display : TFT_eSPI  ILI9488  480x320
 *  Buttons : IO4=PREV  IO5=SELECT  IO6=NEXT
 *  Screens : Dashboard -> Sensors -> NTP Clock -> Battery -> WiFi Info
 *
 *  Dashboard style: pure black, 48px time, 3x3 status grid
 *  Status colours ONLY on values — everything else near-invisible grey
 * ============================================================
 *  TFT_eSPI User_Setup.h:
 *    #define ILI9488_DRIVER
 *    #define TFT_WIDTH  320
 *    #define TFT_HEIGHT 480
 *    setRotation(1) -> 480 wide x 320 tall
 * ============================================================
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>

/* ─────────────────────────────────────────────────────────────
   CREDENTIALS
   ───────────────────────────────────────────────────────────── */
#define WIFI_SSID   "SSID"
#define WIFI_PASS   "PASSWORD"
#define NTP_SERVER  "pool.ntp.org"
#define NTP_SERVER2 "time.google.com"
#define NTP_SERVER3 "time.cloudflare.com"
#define GMT_OFFSET  19800
#define DST_OFFSET  0

/* ─────────────────────────────────────────────────────────────
   PINS
   ───────────────────────────────────────────────────────────── */
#define BTN_PREV      4
#define BTN_SELECT    5
#define BTN_NEXT      6
#define TFT_BL       47
#define PIN_CHARGING 18
#define PIN_CHARGED  17
#define PIN_VIN       8
#define BATT_ADC_PIN  3

#define BATT_ADC_SAMPLES 16
#define BATT_V_DIVIDER   2.0f
#define BATT_ADC_VREF    3.3f
#define BATT_ADC_MAX     4095.0f
#define BATT_V_MAX       4.20f
#define BATT_V_MIN       3.00f

/* ─────────────────────────────────────────────────────────────
   DISPLAY CONSTANTS
   ───────────────────────────────────────────────────────────── */
#define SCR_W  480
#define SCR_H  320
#define HDR_H   26
#define FTR_H   20
#define BODY_Y  HDR_H
#define BODY_H  (SCR_H - HDR_H - FTR_H)

/*
 * Concept C palette (RGB565)
 *
 * Pure black bg, near-invisible borders, white/dim text.
 * Colour ONLY on status values: green=good, amber=warn, red=bad.
 *
 * RGB888 -> RGB565 conversion: R>>3 <<11 | G>>2 <<5 | B>>3
 *
 * #0c0c0f -> 00001 000001 00001 -> 0x0841  (very close, use as BG)
 * #181820 -> 00011 000011 00100 -> 0x18C4  (grid lines / separator)
 * #2e2e38 -> 00101 010111 00111 -> 0x2B87  (dim labels)
 * #d0d0d0 -> 11010 011010 11010 -> 0xD39A  (primary text)
 * #e8e8e8 -> 11101 011101 11101 -> 0xEF7D  (bright time digits)
 * #4ade80 -> 01001 101011 10000 -> 0x4D50  (green - connected/good)
 * #fbbf24 -> 11111 010111 00100 -> 0xFAE4  (amber - warning)
 * #f87171 -> 11111 000111 01110 -> 0xF38E  (red - error)
 */
#define C_BG     0x0841   /* #0c0c0f  background          */
#define C_GRID   0x18C4   /* #181820  1px grid lines       */
#define C_DIM    0x2B87   /* #2e2e38  labels, sub-text     */
#define C_TEXT   0xD39A   /* #d0d0d0  normal values        */
#define C_BRIGHT 0xEF7D   /* #e8e8e8  big clock digits     */
#define C_GREEN  0x4D50   /* #4ade80  good / connected     */
#define C_AMBER  0xFAE4   /* #fbbf24  warning / charging   */
#define C_RED    0xF38E   /* #f87171  error / low batt     */

/* ─────────────────────────────────────────────────────────────
   SCREENS
   ───────────────────────────────────────────────────────────── */
enum Screen {
    SCR_DASHBOARD = 0,
    SCR_SENSORS,
    SCR_NTP_CLOCK,
    SCR_BATTERY,
    SCR_WIFI_INFO,
    SCR_COUNT
};

const char* SCREEN_TITLES[SCR_COUNT] = {
    "DASHBOARD", "SENSORS", "NTP CLOCK", "BATTERY", "WIFI INFO"
};

/* ─────────────────────────────────────────────────────────────
   STATE
   ───────────────────────────────────────────────────────────── */
struct BattState {
    float voltage    = 0.0f;
    int   pct        = 0;
    int   adcRaw     = 0;
    bool  charging   = false;
    bool  charged    = false;
    bool  powered    = false;
    float sessionMin = 9.9f;
    float sessionMax = 0.0f;
    float sessionSum = 0.0f;
    int   sessionN   = 0;
};

struct NTPState {
    bool   synced        = false;
    int    syncCount     = 0;
    time_t lastSyncEpoch = 0;
};

struct SensorData {
    bool  tempValid = false;  float tempC   = 0.0f;
    bool  humValid  = false;  float humPct  = 0.0f;
    bool  presValid = false;  float presHpa = 0.0f;
};

BattState  batt;
NTPState   ntpst;
SensorData sensors;

TFT_eSPI tft = TFT_eSPI();

Screen currentScreen  = SCR_DASHBOARD;
bool   needFullRedraw = true;

unsigned long lastBtnTime     = 0;
unsigned long bootMillis      = 0;
unsigned long lastBattRead    = 0;
unsigned long lastNTPSync     = 0;
unsigned long lastLiveRefresh = 0;

#define DEBOUNCE_MS        180
#define BATT_READ_INTERVAL 5000
#define NTP_SYNC_INTERVAL  600000UL
#define LIVE_REFRESH_MS    1000

/* ─────────────────────────────────────────────────────────────
   UTILITIES
   ───────────────────────────────────────────────────────────── */
bool btnPressed(int pin) {
    if (digitalRead(pin) == LOW && millis() - lastBtnTime > DEBOUNCE_MS) {
        lastBtnTime = millis();
        return true;
    }
    return false;
}

String uptimeStr() {
    unsigned long s = (millis() - bootMillis) / 1000;
    unsigned long h = s / 3600; s %= 3600;
    unsigned long m = s / 60;   s %= 60;
    char buf[12];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, s);
    return String(buf);
}

String timeStr() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    return String(buf);
}

String dateStr() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    const char* days[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
    const char* months[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                             "JUL","AUG","SEP","OCT","NOV","DEC"};
    char buf[32];
    snprintf(buf, sizeof(buf), "%s · %02d %s %04d",
             days[t.tm_wday], t.tm_mday, months[t.tm_mon], t.tm_year + 1900);
    return String(buf);
}

uint16_t rssiColor(int rssi) {
    if (rssi > -65) return C_GREEN;
    if (rssi > -75) return C_AMBER;
    return C_RED;
}

const char* rssiLabel(int rssi) {
    if (rssi > -50) return "Excellent";
    if (rssi > -65) return "Good";
    if (rssi > -75) return "Fair";
    if (rssi > -85) return "Weak";
    return "Poor";
}

int rssiBars(int rssi) {
    if (rssi > -50) return 4;
    if (rssi > -65) return 3;
    if (rssi > -75) return 2;
    if (rssi > -85) return 1;
    return 0;
}

String battStateStr() {
    if (batt.charged)  return "Charged";
    if (batt.charging) return "Charging";
    if (batt.powered)  return "Powered";
    return "On Battery";
}

uint16_t battValueColor() {
    if (batt.charged)  return C_GREEN;
    if (batt.charging) return C_AMBER;
    if (batt.powered)  return C_GREEN;
    if (batt.pct < 15) return C_RED;
    if (batt.pct < 40) return C_AMBER;
    return C_GREEN;
}

/* ─────────────────────────────────────────────────────────────
   BATTERY READ
   ───────────────────────────────────────────────────────────── */
void readBattery() {
    uint32_t sum = 0;
    for (int i = 0; i < BATT_ADC_SAMPLES; i++) {
        sum += analogRead(BATT_ADC_PIN);
        delayMicroseconds(100);
    }
    batt.adcRaw  = (int)(sum / BATT_ADC_SAMPLES);
    batt.voltage = ((float)batt.adcRaw / BATT_ADC_MAX) * BATT_ADC_VREF * BATT_V_DIVIDER;
    float v      = constrain(batt.voltage, BATT_V_MIN, BATT_V_MAX);
    batt.pct     = (int)(((v - BATT_V_MIN) / (BATT_V_MAX - BATT_V_MIN)) * 100.0f);
    batt.charging = (digitalRead(PIN_CHARGING) == LOW);
    batt.charged  = (digitalRead(PIN_CHARGED)  == LOW);
    batt.powered  = (digitalRead(PIN_VIN)      == LOW);
    if (batt.voltage < batt.sessionMin) batt.sessionMin = batt.voltage;
    if (batt.voltage > batt.sessionMax) batt.sessionMax = batt.voltage;
    batt.sessionSum += batt.voltage;
    batt.sessionN++;
}

/* ─────────────────────────────────────────────────────────────
   NTP SYNC
   ───────────────────────────────────────────────────────────── */
void doNTPSync() {
    if (WiFi.status() != WL_CONNECTED) return;

    static bool configured = false;
    if (!configured) {
        configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER, NTP_SERVER2, NTP_SERVER3);
        configured = true;
        Serial.println("[NTP] configTime called");
    } else {
        sntp_restart();
        Serial.println("[NTP] sntp_restart called");
    }

    Serial.print("[NTP] waiting");
    unsigned long t0 = millis();
    while (time(nullptr) < 1000000000UL && millis() - t0 < 10000) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();

    time_t now = time(nullptr);
    Serial.printf("[NTP] epoch=%lu\n", (unsigned long)now);

    if (now > 1000000000UL) {
        ntpst.synced        = true;
        ntpst.syncCount++;
        ntpst.lastSyncEpoch = now;
        lastNTPSync         = millis();
        Serial.println("[NTP] OK");
    } else {
        Serial.println("[NTP] FAILED");
    }
}

/* ─────────────────────────────────────────────────────────────
   HEADER  (26px)
   Title left · nav dots centre · battery % right
   ───────────────────────────────────────────────────────────── */
void drawHeader(const char* title) {
    tft.fillRect(0, 0, SCR_W, HDR_H, C_BG);
    tft.drawFastHLine(0, HDR_H - 1, SCR_W, C_GRID);

    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.setCursor(8, 9);
    tft.print(title);

    int dotX = (SCR_W - SCR_COUNT * 10) / 2;
    for (int i = 0; i < SCR_COUNT; i++) {
        uint16_t col = (i == (int)currentScreen) ? C_TEXT : C_GRID;
        tft.fillCircle(dotX + i * 10, HDR_H / 2, 2, col);
    }

    char bpct[8];
    snprintf(bpct, sizeof(bpct), "%d%%", batt.pct);
    int bw = strlen(bpct) * 6;
    tft.setTextColor(battValueColor());
    tft.setCursor(SCR_W - bw - 8, 9);
    tft.print(bpct);
}

/* ─────────────────────────────────────────────────────────────
   FOOTER  (20px)
   ───────────────────────────────────────────────────────────── */
void drawFooter(const char* left, const char* mid, const char* right) {
    tft.fillRect(0, SCR_H - FTR_H, SCR_W, FTR_H, C_BG);
    tft.drawFastHLine(0, SCR_H - FTR_H, SCR_W, C_GRID);
    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.setCursor(8, SCR_H - FTR_H + 6);
    tft.print(left);
    int mw = strlen(mid) * 6;
    tft.setCursor((SCR_W - mw) / 2, SCR_H - FTR_H + 6);
    tft.print(mid);
    int rw = strlen(right) * 6;
    tft.setCursor(SCR_W - rw - 8, SCR_H - FTR_H + 6);
    tft.print(right);
}

/* ─────────────────────────────────────────────────────────────
   GRID CELL HELPER
   Draws a single cell with a dim label and a coloured value.
   sub is optional (pass "" to skip).
   ───────────────────────────────────────────────────────────── */
void drawCell(int x, int y, int w, int h,
              const char* label, const char* value, uint16_t valColor,
              const char* sub)
{
    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.setCursor(x + 6, y + 5);
    tft.print(label);

    tft.setTextSize(2);
    tft.setTextColor(valColor);
    tft.setCursor(x + 6, y + 17);
    tft.print(value);

    if (sub && sub[0]) {
        tft.setTextSize(1);
        tft.setTextColor(C_DIM);
        tft.setCursor(x + 6, y + h - 11);
        tft.print(sub);
    }
}

/* ─────────────────────────────────────────────────────────────
   SCREEN 0 — DASHBOARD  (Concept C)
   ───────────────────────────────────────────────────────────── */

/*
 * Layout (480 x 274 body):
 *
 *  [  48px clock + 12px date = 68px  ]
 *  [  1px gap line                   ]
 *  [  3 x 3 grid  = 205px            ]
 *  Total ~274  body height ok
 *
 * Grid: 3 cols x 3 rows, 1px separators from C_GRID
 *   col widths: 160 | 160 | 160  (480 / 3)
 *   row heights: 68 | 68 | 69    (205 / 3, last row gets remainder)
 */
#define GRID_COLS 3
#define GRID_ROWS 3
#define CELL_W    160
#define CELL_H     68
#define CLOCK_H    62   /* big time + date block height */
#define GRID_Y    (BODY_Y + CLOCK_H + 2)

void drawGridLines() {
    int gridBottom = GRID_Y + CELL_H * GRID_ROWS + (GRID_ROWS - 1);
    /* horizontal separators */
    for (int r = 1; r < GRID_ROWS; r++) {
        int gy = GRID_Y + r * (CELL_H + 1) - 1;
        tft.drawFastHLine(0, gy, SCR_W, C_GRID);
    }
    /* vertical separators */
    for (int c = 1; c < GRID_COLS; c++) {
        int gx = c * (CELL_W + 1) - 1;
        tft.drawFastVLine(gx, GRID_Y, gridBottom - GRID_Y, C_GRID);
    }
}

void drawDashboard() {
    tft.fillRect(0, BODY_Y, SCR_W, BODY_H, C_BG);
    drawHeader("DASHBOARD");

    /* ── Big clock ── */
    String t = timeStr();
    /* textSize(4): each char 6*4=24px wide, 8*4=32px tall */
    tft.setTextSize(4);
    tft.setTextColor(C_BRIGHT);
    tft.setCursor(8, BODY_Y + 6);
    tft.print(t);

    /* ── Date sub-line ── */
    String d = dateStr();
    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.setCursor(10, BODY_Y + 46);
    tft.print(d);

    /* ── Thin separator ── */
    tft.drawFastHLine(0, BODY_Y + CLOCK_H, SCR_W, C_GRID);

    /* ── Grid lines ── */
    drawGridLines();

    /* ── Cell data ── */
    /* Row 0 */
    /* WIFI */
    {
        int cx = 0, cy = GRID_Y;
        if (WiFi.status() == WL_CONNECTED) {
            String ssid = WiFi.SSID();
            if (ssid.length() > 10) ssid = ssid.substring(0, 9) + "~";
            char sub[20];
            snprintf(sub, sizeof(sub), "%ddBm %s", WiFi.RSSI(), rssiLabel(WiFi.RSSI()));
            drawCell(cx, cy, CELL_W, CELL_H, "WIFI",
                     ssid.c_str(), C_GREEN, sub);
        } else {
            drawCell(cx, cy, CELL_W, CELL_H, "WIFI", "No WiFi", C_RED, "");
        }
    }
    /* BATTERY */
    {
        int cx = CELL_W + 1, cy = GRID_Y;
        char vbuf[10];
        snprintf(vbuf, sizeof(vbuf), "%.2fV", batt.voltage);
        char sub[20];
        snprintf(sub, sizeof(sub), "%d%% %s", batt.pct, battStateStr().c_str());
        drawCell(cx, cy, CELL_W, CELL_H, "BATTERY", vbuf, battValueColor(), sub);
    }
    /* UPTIME */
    {
        int cx = (CELL_W + 1) * 2, cy = GRID_Y;
        drawCell(cx, cy, CELL_W, CELL_H, "UPTIME",
                 uptimeStr().c_str(), C_TEXT, "hh:mm:ss");
    }

    /* Row 1 */
    int row1Y = GRID_Y + CELL_H + 1;
    /* TEMP */
    {
        int cx = 0, cy = row1Y;
        if (sensors.tempValid) {
            char tb[10]; snprintf(tb, sizeof(tb), "%.1fC", sensors.tempC);
            drawCell(cx, cy, CELL_W, CELL_H, "TEMP", tb, C_AMBER, "");
        } else {
            drawCell(cx, cy, CELL_W, CELL_H, "TEMP", "--", C_DIM, "pending");
        }
    }
    /* HUMIDITY */
    {
        int cx = CELL_W + 1, cy = row1Y;
        if (sensors.humValid) {
            char hb[10]; snprintf(hb, sizeof(hb), "%.0f%%", sensors.humPct);
            drawCell(cx, cy, CELL_W, CELL_H, "HUMIDITY", hb, C_TEXT, "");
        } else {
            drawCell(cx, cy, CELL_W, CELL_H, "HUMIDITY", "--", C_DIM, "pending");
        }
    }
    /* PRESSURE */
    {
        int cx = (CELL_W + 1) * 2, cy = row1Y;
        if (sensors.presValid) {
            char pb[12]; snprintf(pb, sizeof(pb), "%.0fhPa", sensors.presHpa);
            drawCell(cx, cy, CELL_W, CELL_H, "PRESSURE", pb, C_TEXT, "");
        } else {
            drawCell(cx, cy, CELL_W, CELL_H, "PRESSURE", "--", C_DIM, "pending");
        }
    }

    /* Row 2 */
    int row2Y = GRID_Y + (CELL_H + 1) * 2;
    /* NTP SYNC */
    {
        int cx = 0, cy = row2Y;
        char sub[12]; snprintf(sub, sizeof(sub), "count: %d", ntpst.syncCount);
        drawCell(cx, cy, CELL_W, CELL_H, "NTP SYNC",
                 ntpst.synced ? "Synced" : "No Sync",
                 ntpst.synced ? C_GREEN : C_RED,
                 ntpst.synced ? sub : "");
    }
    /* BATT ADC */
    {
        int cx = CELL_W + 1, cy = row2Y;
        char ab[8]; snprintf(ab, sizeof(ab), "%d", batt.adcRaw);
        drawCell(cx, cy, CELL_W, CELL_H, "BATT ADC", ab, C_TEXT, "12-bit raw");
    }
    /* POWER */
    {
        int cx = (CELL_W + 1) * 2, cy = row2Y;
        drawCell(cx, cy, CELL_W, CELL_H, "POWER",
                 battStateStr().c_str(), battValueColor(), "");
    }

    drawFooter("< PREV", "SEL = REFRESH", "NEXT >");
}

/* ─────────────────────────────────────────────────────────────
   SCREEN 1 — SENSORS
   ───────────────────────────────────────────────────────────── */
void drawSensors() {
    tft.fillRect(0, BODY_Y, SCR_W, BODY_H, C_BG);
    drawHeader("SENSORS");

    int y   = BODY_Y + 6;
    int cw  = 232;
    int ch  = 90;
    int gap = 6;

    auto sensorCard = [&](int x, int cy, const char* label,
                           bool valid, const char* valStr, uint16_t col,
                           const char* hint) {
        tft.fillRect(x, cy, cw, ch, C_BG);
        tft.drawRect(x, cy, cw, ch, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(x + 8, cy + 7); tft.print(label);
        if (valid) {
            tft.setTextSize(3); tft.setTextColor(col);
            tft.setCursor(x + 8, cy + 22); tft.print(valStr);
        } else {
            tft.setTextSize(2); tft.setTextColor(C_GRID);
            tft.setCursor(x + 8, cy + 28); tft.print("Sensor pending");
            tft.setTextSize(1); tft.setTextColor(C_DIM);
            tft.setCursor(x + 8, cy + 55); tft.print(hint);
        }
    };

    sensorCard(4, y, "TEMPERATURE",
               sensors.tempValid,
               sensors.tempValid ? ([]() { static char b[10];
                   snprintf(b, sizeof(b), "%.1f C", 0.0f); return b; })() : "",
               C_AMBER, "DHT22 / BME280");

    if (sensors.tempValid) {
        char tb[10]; snprintf(tb, sizeof(tb), "%.1f C", sensors.tempC);
        tft.fillRect(4, y, cw, ch, C_BG);
        tft.drawRect(4, y, cw, ch, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 7); tft.print("TEMPERATURE");
        tft.setTextSize(3); tft.setTextColor(C_AMBER);
        tft.setCursor(12, y + 22); tft.print(tb);
    }

    sensorCard(4 + cw + gap, y, "HUMIDITY",
               sensors.humValid,
               sensors.humValid ? ([]() { static char b[10];
                   snprintf(b, sizeof(b), "%.0f %%", 0.0f); return b; })() : "",
               C_TEXT, "DHT22 / SHT31");

    if (sensors.humValid) {
        char hb[10]; snprintf(hb, sizeof(hb), "%.0f %%", sensors.humPct);
        int hx = 4 + cw + gap;
        tft.fillRect(hx, y, cw, ch, C_BG);
        tft.drawRect(hx, y, cw, ch, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(hx + 8, y + 7); tft.print("HUMIDITY");
        tft.setTextSize(3); tft.setTextColor(C_TEXT);
        tft.setCursor(hx + 8, y + 22); tft.print(hb);
    }

    y += ch + gap;

    sensorCard(4, y, "PRESSURE",
               sensors.presValid,
               sensors.presValid ? ([]() { static char b[12];
                   snprintf(b, sizeof(b), "%.1f hPa", 0.0f); return b; })() : "",
               C_TEXT, "BMP280 / BME280");

    if (sensors.presValid) {
        char pb[12]; snprintf(pb, sizeof(pb), "%.1f hPa", sensors.presHpa);
        tft.fillRect(4, y, cw, ch, C_BG);
        tft.drawRect(4, y, cw, ch, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 7); tft.print("PRESSURE");
        tft.setTextSize(3); tft.setTextColor(C_TEXT);
        tft.setCursor(12, y + 22); tft.print(pb);
    }

    /* Battery ADC — always live */
    int bx = 4 + cw + gap;
    tft.fillRect(bx, y, cw, ch, C_BG);
    tft.drawRect(bx, y, cw, ch, C_GRID);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(bx + 8, y + 7); tft.print("BATTERY ADC");
    char vb[10]; snprintf(vb, sizeof(vb), "%.2f V", batt.voltage);
    tft.setTextSize(3); tft.setTextColor(battValueColor());
    tft.setCursor(bx + 8, y + 22); tft.print(vb);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(bx + 8, y + 62);
    tft.printf("Raw: %d   %d%%", batt.adcRaw, batt.pct);
    /* thin progress bar */
    tft.fillRect(bx + 8, y + 76, cw - 16, 4, C_GRID);
    int fw = map(constrain(batt.pct, 0, 100), 0, 100, 0, cw - 16);
    if (fw > 0) tft.fillRect(bx + 8, y + 76, fw, 4, battValueColor());

    drawFooter("< PREV", "SEL = REFRESH", "NEXT >");
}

/* ─────────────────────────────────────────────────────────────
   SCREEN 2 — NTP CLOCK
   ───────────────────────────────────────────────────────────── */
void drawNTPClock() {
    tft.fillRect(0, BODY_Y, SCR_W, BODY_H, C_BG);
    drawHeader("NTP CLOCK");

    /* Big clock */
    String t = timeStr();
    tft.setTextSize(4); tft.setTextColor(C_BRIGHT);
    int tw = t.length() * 24;
    tft.setCursor((SCR_W - tw) / 2, BODY_Y + 8);
    tft.print(t);

    String d = dateStr();
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    int dw = d.length() * 6;
    tft.setCursor((SCR_W - dw) / 2, BODY_Y + 48);
    tft.print(d);

    tft.drawFastHLine(0, BODY_Y + 62, SCR_W, C_GRID);

    int y = BODY_Y + 68;

    /* 3-col info row */
    int cw = (SCR_W - 2) / 3;
    tft.drawFastVLine(cw,     y, 110, C_GRID);
    tft.drawFastVLine(cw * 2, y, 110, C_GRID);

    /* NTP server */
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(8, y + 4); tft.print("NTP SERVER");
    tft.setTextColor(C_TEXT);
    tft.setCursor(8, y + 16); tft.print(NTP_SERVER);
    tft.setTextColor(C_DIM);
    tft.setCursor(8, y + 28); tft.print(NTP_SERVER2);

    /* Timezone */
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(cw + 8, y + 4); tft.print("TIMEZONE");
    tft.setTextSize(2); tft.setTextColor(C_TEXT);
    tft.setCursor(cw + 8, y + 16);
    tft.printf("UTC+%d:%02d", GMT_OFFSET / 3600, (GMT_OFFSET % 3600) / 60);

    /* Status */
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(cw * 2 + 8, y + 4); tft.print("STATUS");
    tft.setTextSize(2);
    tft.setTextColor(ntpst.synced ? C_GREEN : C_RED);
    tft.setCursor(cw * 2 + 8, y + 16);
    tft.print(ntpst.synced ? "Synced" : "No Sync");

    y += 50;
    tft.drawFastHLine(0, y, SCR_W, C_GRID);
    y += 6;

    /* Last sync | Count | Epoch */
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(8, y + 4); tft.print("LAST SYNC");
    tft.setTextColor(C_TEXT);
    tft.setCursor(8, y + 16);
    if (ntpst.lastSyncEpoch > 0) {
        unsigned long ago = (millis() - lastNTPSync) / 1000;
        if      (ago < 60)   tft.printf("%lus ago", ago);
        else if (ago < 3600) tft.printf("%lum ago", ago / 60);
        else                 tft.printf("%luh ago", ago / 3600);
    } else {
        tft.print("Never");
    }

    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(cw + 8, y + 4); tft.print("SYNC COUNT");
    tft.setTextSize(2); tft.setTextColor(C_TEXT);
    tft.setCursor(cw + 8, y + 16); tft.printf("%d", ntpst.syncCount);

    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(cw * 2 + 8, y + 4); tft.print("EPOCH");
    tft.setTextColor(C_TEXT);
    tft.setCursor(cw * 2 + 8, y + 16);
    tft.printf("%lu", (unsigned long)time(nullptr));

    drawFooter("< PREV", "SEL = FORCE SYNC", "NEXT >");
}

void updateClockPartial() {
    tft.fillRect(0, BODY_Y, SCR_W, 62, C_BG);
    String t = timeStr();
    tft.setTextSize(4); tft.setTextColor(C_BRIGHT);
    int tw = t.length() * 24;
    tft.setCursor((SCR_W - tw) / 2, BODY_Y + 8);
    tft.print(t);
    String d = dateStr();
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    int dw = d.length() * 6;
    tft.setCursor((SCR_W - dw) / 2, BODY_Y + 48);
    tft.print(d);
}

/* ─────────────────────────────────────────────────────────────
   SCREEN 3 — BATTERY
   ───────────────────────────────────────────────────────────── */
void drawBattery() {
    tft.fillRect(0, BODY_Y, SCR_W, BODY_H, C_BG);
    drawHeader("BATTERY");

    int y = BODY_Y + 8;

    /* Big voltage */
    tft.fillRect(4, y, 236, 100, C_BG);
    tft.drawRect(4, y, 236, 100, C_GRID);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(12, y + 7); tft.print("VOLTAGE");
    char vb[10]; snprintf(vb, sizeof(vb), "%.2fV", batt.voltage);
    tft.setTextSize(4); tft.setTextColor(battValueColor());
    tft.setCursor(12, y + 20); tft.print(vb);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(12, y + 74); tft.print("3.0");
    tft.setCursor(66, y + 74); tft.print("3.5");
    tft.setCursor(120, y + 74); tft.print("4.0");
    tft.setCursor(178, y + 74); tft.print("4.2V");
    tft.fillRect(12, y + 86, 212, 4, C_GRID);
    int bfw = map(constrain(batt.pct, 0, 100), 0, 100, 0, 212);
    if (bfw > 0) tft.fillRect(12, y + 86, bfw, 4, battValueColor());

    /* PCT */
    tft.fillRect(244, y, 232, 48, C_BG);
    tft.drawRect(244, y, 232, 48, C_GRID);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(252, y + 7); tft.print("PERCENTAGE");
    tft.setTextSize(3); tft.setTextColor(battValueColor());
    tft.setCursor(252, y + 20); tft.printf("%d%%", batt.pct);

    /* ADC raw */
    tft.fillRect(244, y + 52, 232, 48, C_BG);
    tft.drawRect(244, y + 52, 232, 48, C_GRID);
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(252, y + 59); tft.print("ADC RAW (12-bit)");
    tft.setTextSize(2); tft.setTextColor(C_TEXT);
    tft.setCursor(252, y + 72); tft.printf("%d", batt.adcRaw);

    y += 108;

    /* Status pins — 3 equal tiles */
    int pw = (SCR_W - 8 - 2) / 3;
    for (int i = 0; i < 3; i++) {
        int bx = 4 + i * (pw + 1);
        tft.fillRect(bx, y, pw, 52, C_BG);
        tft.drawRect(bx, y, pw, 52, C_GRID);
    }
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(12, y + 7); tft.print("CHARGING");
    tft.setTextSize(2); tft.setTextColor(batt.charging ? C_AMBER : C_DIM);
    tft.setCursor(12, y + 20); tft.print(batt.charging ? "YES" : "NO");

    int px2 = 4 + pw + 1;
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(px2 + 8, y + 7); tft.print("CHARGED");
    tft.setTextSize(2); tft.setTextColor(batt.charged ? C_GREEN : C_DIM);
    tft.setCursor(px2 + 8, y + 20); tft.print(batt.charged ? "FULL" : "NO");

    int px3 = 4 + (pw + 1) * 2;
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(px3 + 8, y + 7); tft.print("USB / VIN");
    tft.setTextSize(2); tft.setTextColor(batt.powered ? C_GREEN : C_DIM);
    tft.setCursor(px3 + 8, y + 20); tft.print(batt.powered ? "PRESENT" : "ABSENT");

    y += 56;

    /* Session stats — 3 equal tiles */
    for (int i = 0; i < 3; i++) {
        int bx = 4 + i * (pw + 1);
        tft.fillRect(bx, y, pw, 28, C_BG);
        tft.drawRect(bx, y, pw, 28, C_GRID);
    }
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(12, y + 4); tft.print("SESSION MIN");
    tft.setTextColor(C_TEXT);
    tft.setCursor(12, y + 16);
    tft.printf("%.3fV", batt.sessionMin < 9.0f ? batt.sessionMin : 0.0f);

    tft.setTextColor(C_DIM);
    tft.setCursor(px2 + 8, y + 4); tft.print("SESSION MAX");
    tft.setTextColor(C_TEXT);
    tft.setCursor(px2 + 8, y + 16); tft.printf("%.3fV", batt.sessionMax);

    tft.setTextColor(C_DIM);
    tft.setCursor(px3 + 8, y + 4); tft.print("SESSION AVG");
    tft.setTextColor(C_TEXT);
    tft.setCursor(px3 + 8, y + 16);
    float avg = batt.sessionN > 0 ? batt.sessionSum / batt.sessionN : 0.0f;
    tft.printf("%.3fV", avg);

    drawFooter("< PREV", "SEL = REFRESH", "NEXT >");
}

/* ─────────────────────────────────────────────────────────────
   SCREEN 4 — WIFI INFO
   ───────────────────────────────────────────────────────────── */
void drawWifiInfo() {
    tft.fillRect(0, BODY_Y, SCR_W, BODY_H, C_BG);
    drawHeader("WIFI INFO");

    int y = BODY_Y + 6;
    bool conn = WiFi.status() == WL_CONNECTED;

    /* Status banner */
    tft.fillRect(4, y, 472, 32, C_BG);
    tft.drawRect(4, y, 472, 32, conn ? C_GREEN : C_RED);
    tft.setTextSize(2);
    tft.setTextColor(conn ? C_GREEN : C_RED);
    tft.setCursor(12, y + 8);
    tft.print(conn ? "Connected" : "Disconnected");
    if (conn) {
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(160, y + 12); tft.print(WiFi.SSID());
    }

    y += 38;

    if (conn) {
        int cw2 = (SCR_W - 8 - 1) / 2;
        /* IP | Gateway */
        tft.fillRect(4,          y, cw2, 44, C_BG); tft.drawRect(4,          y, cw2, 44, C_GRID);
        tft.fillRect(4 + cw2 + 1, y, cw2, 44, C_BG); tft.drawRect(4 + cw2 + 1, y, cw2, 44, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 6); tft.print("IP ADDRESS");
        tft.setTextSize(2); tft.setTextColor(C_TEXT);
        tft.setCursor(12, y + 18); tft.print(WiFi.localIP().toString());
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(4 + cw2 + 9, y + 6); tft.print("GATEWAY");
        tft.setTextSize(2); tft.setTextColor(C_TEXT);
        tft.setCursor(4 + cw2 + 9, y + 18); tft.print(WiFi.gatewayIP().toString());

        y += 48;

        /* Signal | Channel | Subnet | DNS */
        int qw = (SCR_W - 8 - 3) / 4;
        for (int i = 0; i < 4; i++) {
            int bx = 4 + i * (qw + 1);
            tft.fillRect(bx, y, qw, 44, C_BG);
            tft.drawRect(bx, y, qw, 44, C_GRID);
        }
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 6); tft.print("SIGNAL");
        tft.setTextSize(2); tft.setTextColor(rssiColor(WiFi.RSSI()));
        tft.setCursor(12, y + 18); tft.printf("%ddBm", WiFi.RSSI());
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 36); tft.print(rssiLabel(WiFi.RSSI()));

        int q2 = 4 + qw + 1;
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(q2 + 6, y + 6); tft.print("CHANNEL");
        tft.setTextSize(2); tft.setTextColor(C_TEXT);
        tft.setCursor(q2 + 6, y + 18); tft.printf("CH%d", WiFi.channel());

        int q3 = 4 + (qw + 1) * 2;
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(q3 + 6, y + 6); tft.print("SUBNET");
        tft.setTextColor(C_TEXT);
        tft.setCursor(q3 + 6, y + 18); tft.print(WiFi.subnetMask().toString());

        int q4 = 4 + (qw + 1) * 3;
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(q4 + 6, y + 6); tft.print("DNS");
        tft.setTextColor(C_TEXT);
        tft.setCursor(q4 + 6, y + 18); tft.print(WiFi.dnsIP().toString());

        y += 48;

        /* BSSID */
        tft.fillRect(4, y, 472, 20, C_BG); tft.drawRect(4, y, 472, 20, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 6); tft.print("BSSID:");
        tft.setTextColor(C_TEXT);
        tft.setCursor(58, y + 6); tft.print(WiFi.BSSIDstr());

        y += 24;

        /* MAC */
        tft.fillRect(4, y, 472, 20, C_BG); tft.drawRect(4, y, 472, 20, C_GRID);
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(12, y + 6); tft.print("MAC:");
        tft.setTextColor(C_TEXT);
        tft.setCursor(46, y + 6); tft.print(WiFi.macAddress());

    } else {
        tft.setTextSize(1); tft.setTextColor(C_DIM);
        tft.setCursor(120, BODY_Y + 130); tft.print("Not connected to:");
        tft.setTextColor(C_TEXT);
        tft.setCursor(120, BODY_Y + 148); tft.print(WIFI_SSID);
        tft.setTextColor(C_DIM);
        tft.setCursor(120, BODY_Y + 166); tft.print("Restart device to retry");
    }

    drawFooter("< PREV", "SEL = REFRESH", "NEXT >");
}

/* ─────────────────────────────────────────────────────────────
   SCREEN DISPATCH
   ───────────────────────────────────────────────────────────── */
void drawCurrentScreen() {
    switch (currentScreen) {
        case SCR_DASHBOARD: drawDashboard(); break;
        case SCR_SENSORS:   drawSensors();   break;
        case SCR_NTP_CLOCK: drawNTPClock();  break;
        case SCR_BATTERY:   drawBattery();   break;
        case SCR_WIFI_INFO: drawWifiInfo();  break;
        default: break;
    }
    needFullRedraw = false;
}

/* ─────────────────────────────────────────────────────────────
   SELECT ACTION
   ───────────────────────────────────────────────────────────── */
void handleSelect() {
    switch (currentScreen) {
        case SCR_DASHBOARD:
        case SCR_SENSORS:
        case SCR_BATTERY:
            readBattery();
            needFullRedraw = true;
            break;
        case SCR_NTP_CLOCK:
            doNTPSync();
            needFullRedraw = true;
            break;
        case SCR_WIFI_INFO:
            needFullRedraw = true;
            break;
        default: break;
    }
}

/* ─────────────────────────────────────────────────────────────
   SETUP
   ───────────────────────────────────────────────────────────── */
void setup() {
    Serial.begin(115200);
    bootMillis = millis();

    pinMode(BTN_PREV,     INPUT_PULLUP);
    pinMode(BTN_SELECT,   INPUT_PULLUP);
    pinMode(BTN_NEXT,     INPUT_PULLUP);
    pinMode(PIN_CHARGING, INPUT_PULLUP);
    pinMode(PIN_CHARGED,  INPUT_PULLUP);
    pinMode(PIN_VIN,      INPUT_PULLUP);
    pinMode(TFT_BL,       OUTPUT);
    digitalWrite(TFT_BL,  HIGH);

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(C_BG);

    /* Splash */
    tft.setTextSize(2); tft.setTextColor(C_TEXT);
    tft.setCursor(120, 100); tft.print("ESP32-S3 HUB");
    tft.setTextSize(1); tft.setTextColor(C_DIM);
    tft.setCursor(120, 124); tft.print("initialising...");

    readBattery();
    tft.setTextColor(C_GREEN);
    tft.setCursor(120, 140); tft.print("battery ok");

    tft.setTextColor(C_DIM);
    tft.setCursor(120, 156); tft.print("connecting wifi...");

    WiFi.mode(WIFI_STA);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE,
                IPAddress(8, 8, 8, 8),
                IPAddress(8, 8, 4, 4));
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000)
        delay(300);

    if (WiFi.status() == WL_CONNECTED) {
        tft.setTextColor(C_GREEN);
        tft.setCursor(120, 172);
        tft.print("wifi ok  " + WiFi.localIP().toString());

        tft.setTextColor(C_DIM);
        tft.setCursor(120, 188); tft.print("syncing ntp...");
        doNTPSync();

        tft.setTextColor(ntpst.synced ? C_GREEN : C_RED);
        tft.setCursor(120, 204);
        tft.print(ntpst.synced ? "ntp ok" : "ntp failed");
    } else {
        tft.setTextColor(C_RED);
        tft.setCursor(120, 172); tft.print("wifi failed");
    }

    delay(600);
    tft.fillScreen(C_BG);
    currentScreen  = SCR_DASHBOARD;
    needFullRedraw = true;
}

/* ─────────────────────────────────────────────────────────────
   LOOP
   ───────────────────────────────────────────────────────────── */
void loop() {

    if (btnPressed(BTN_PREV)) {
        currentScreen  = (Screen)(((int)currentScreen - 1 + SCR_COUNT) % SCR_COUNT);
        needFullRedraw = true;
    }
    if (btnPressed(BTN_NEXT)) {
        currentScreen  = (Screen)(((int)currentScreen + 1) % SCR_COUNT);
        needFullRedraw = true;
    }
    if (btnPressed(BTN_SELECT)) {
        handleSelect();
    }

    /* Battery read every 5 s */
    if (millis() - lastBattRead >= BATT_READ_INTERVAL) {
        readBattery();
        lastBattRead = millis();
    }

    /* NTP re-sync every 10 min */
    if (WiFi.status() == WL_CONNECTED &&
        millis() - lastNTPSync >= NTP_SYNC_INTERVAL) {
        doNTPSync();
    }

    /* 1-second partial refresh */
    if (!needFullRedraw && millis() - lastLiveRefresh >= LIVE_REFRESH_MS) {
        lastLiveRefresh = millis();

        if (currentScreen == SCR_NTP_CLOCK) {
            updateClockPartial();
            drawHeader("NTP CLOCK");

        } else if (currentScreen == SCR_DASHBOARD) {
            /*
             * Partial update: only the big clock area.
             * Clock: textSize(4) at x=8, y=BODY_Y+6 → 32px tall → bottom BODY_Y+38
             * Date:  textSize(1) at y=BODY_Y+46     →  8px tall → bottom BODY_Y+54
             * Clear BODY_Y to BODY_Y+56, then redraw both lines.
             */
            tft.fillRect(0, BODY_Y, SCR_W, 56, C_BG);
            tft.setTextSize(4); tft.setTextColor(C_BRIGHT);
            tft.setCursor(8, BODY_Y + 6);
            tft.print(timeStr());
            tft.setTextSize(1); tft.setTextColor(C_DIM);
            tft.setCursor(10, BODY_Y + 46);
            tft.print(dateStr());
            drawHeader("DASHBOARD");

        } else {
            drawHeader(SCREEN_TITLES[(int)currentScreen]);
        }
    }

    /* Full redraw */
    if (needFullRedraw) {
        drawCurrentScreen();
        lastLiveRefresh = millis();
    }
}