/*
 * ESP32-S3 Smart Hub — Full Production Firmware
 * UI: LVGL 8.x + TFT_eSPI
 * Features: Dashboard, Clock (NTP), Battery, WiFi, SD, Buzzer, Buttons
 * Author: Anshuman Tripathy
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <lvgl.h>
#include "config.h"

// ─────────────────────────────────────────
// OBJECTS
// ─────────────────────────────────────────
TFT_eSPI    tft;
SPIClass    sdSPI(HSPI);
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, 60000);

// LVGL buffers (in PSRAM)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t*        buf1;
static lv_color_t*        buf2;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;

// ─────────────────────────────────────────
// UI OBJECTS
// ─────────────────────────────────────────
// Tabs
lv_obj_t* tabview;
lv_obj_t* tab_dash;
lv_obj_t* tab_sensor;
lv_obj_t* tab_settings;

// Dashboard widgets
lv_obj_t* lbl_time;
lv_obj_t* lbl_date;
lv_obj_t* lbl_wifi_status;
lv_obj_t* bar_battery;
lv_obj_t* lbl_battery;
lv_obj_t* lbl_charge_status;
lv_obj_t* arc_battery;

// Sensor widgets
lv_obj_t* lbl_temp;
lv_obj_t* lbl_humidity;
lv_obj_t* lbl_pressure;
lv_obj_t* lbl_aqi;
lv_obj_t* lbl_sensor_time;
lv_obj_t* chart_temp;
lv_chart_series_t* ser_temp;

// Settings widgets
lv_obj_t* sw_buzzer;
lv_obj_t* sw_backlight;
lv_obj_t* slider_brightness;
lv_obj_t* lbl_sd_info;
lv_obj_t* lbl_ip;

// Notification bar
lv_obj_t* notif_bar;
lv_obj_t* lbl_notif;

// ─────────────────────────────────────────
// STATE
// ─────────────────────────────────────────
struct SensorData {
  float temp     = 0;
  float humidity = 0;
  float pressure = 0;
  float aqi      = 0;
  bool  fresh    = false;
  char  time[20] = "--:--";
};

struct BatteryState {
  float   voltage = 0;
  int     percent = 0;
  bool    charging = false;
  bool    full     = false;
  bool    usbPower = false;
};

SensorData   sensor;
BatteryState battery;
bool         buzzerEnabled = true;
bool         sdMounted     = false;
int          brightness    = 200;

// Timers
unsigned long lastBattUpdate   = 0;
unsigned long lastNTPUpdate    = 0;
unsigned long lastUIUpdate     = 0;
unsigned long lastSensorCheck  = 0;
unsigned long lastLogWrite     = 0;

// ─────────────────────────────────────────
// BUZZER
// ─────────────────────────────────────────
void beep(int freq = 1000, int ms = 80) {
  if (!buzzerEnabled) return;
  tone(BUZZER_PIN, freq, ms);
}

void beepSuccess() { beep(1500, 60); delay(80); beep(2000, 60); }
void beepFail()    { beep(300, 300); }
void beepClick()   { beep(1200, 30); }

// ─────────────────────────────────────────
// BATTERY
// ─────────────────────────────────────────
void updateBattery() {
  int sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(BATT_ADC); delay(2); }
  float raw  = sum / 16.0;
  battery.voltage  = (raw / 4095.0) * 3.3 * 2.0;
  battery.percent  = constrain(
    map((int)(battery.voltage * 1000), BATT_MIN_MV, BATT_MAX_MV, 0, 100), 0, 100);
  battery.charging = digitalRead(STAT1_PIN) == LOW;
  battery.full     = digitalRead(STAT2_PIN) == LOW;
  battery.usbPower = digitalRead(PG_PIN)    == LOW;
}

// ─────────────────────────────────────────
// SD CARD
// ─────────────────────────────────────────
bool initSD() {
  if (digitalRead(SD_CD) == HIGH) return false;
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  return SD.begin(SD_CS, sdSPI);
}

void logToSD(const char* entry) {
  if (!sdMounted) return;
  File f = SD.open("/hub_log.csv", FILE_APPEND);
  if (!f) return;
  f.println(entry);
  f.close();
}

String getSDInfo() {
  if (!sdMounted) return "No SD";
  return String(SD.usedBytes() / (1024*1024)) + "/" +
         String(SD.totalBytes() / (1024*1024)) + "MB";
}

// ─────────────────────────────────────────
// WIFI
// ─────────────────────────────────────────
void initWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    timeClient.update();
    beepSuccess();
  }
}

// ─────────────────────────────────────────
// LVGL DISPLAY FLUSH
// ─────────────────────────────────────────
void lvgl_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)color_p, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(drv);
}

// ─────────────────────────────────────────
// LVGL TOUCH INPUT
// ─────────────────────────────────────────
void lvgl_touch(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  uint16_t tx, ty;
  bool touched = tft.getTouch(&tx, &ty);
  if (touched) {
    data->state   = LV_INDEV_STATE_PR;
    data->point.x = tx;
    data->point.y = ty;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// ─────────────────────────────────────────
// NOTIFICATION
// ─────────────────────────────────────────
void showNotif(const char* msg, lv_color_t color = lv_color_hex(0x2196F3)) {
  lv_obj_set_style_bg_color(notif_bar, color, 0);
  lv_label_set_text(lbl_notif, msg);
  lv_obj_clear_flag(notif_bar, LV_OBJ_FLAG_HIDDEN);
  // Auto hide after 3 seconds via timer
  static lv_timer_t* notif_timer = nullptr;
  if (notif_timer) lv_timer_del(notif_timer);
  notif_timer = lv_timer_create([](lv_timer_t* t) {
    lv_obj_add_flag(notif_bar, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t);
  }, 3000, nullptr);
}

// ─────────────────────────────────────────
// BUILD UI
// ─────────────────────────────────────────
void buildUI() {
  // Dark theme
  lv_theme_t* th = lv_theme_default_init(
    lv_disp_get_default(),
    lv_palette_main(LV_PALETTE_BLUE),
    lv_palette_main(LV_PALETTE_CYAN),
    true, &lv_font_montserrat_14);
  lv_disp_set_theme(lv_disp_get_default(), th);

  // Root screen
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1117), 0);

  // Notification bar (hidden by default)
  notif_bar = lv_obj_create(scr);
  lv_obj_set_size(notif_bar, 480, 28);
  lv_obj_align(notif_bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_radius(notif_bar, 0, 0);
  lv_obj_set_style_border_width(notif_bar, 0, 0);
  lv_obj_add_flag(notif_bar, LV_OBJ_FLAG_HIDDEN);
  lbl_notif = lv_label_create(notif_bar);
  lv_obj_center(lbl_notif);
  lv_obj_set_style_text_color(lbl_notif, lv_color_white(), 0);

  // Tabview
  tabview = lv_tabview_create(scr, LV_DIR_TOP, 40);
  lv_obj_set_size(tabview, 480, 320);
  lv_obj_align(tabview, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(tabview, lv_color_hex(0x0D1117), 0);

  // Tab buttons style
  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabview);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x161B22), 0);
  lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x58A6FF), LV_STATE_CHECKED);

  // Create tabs
  tab_dash     = lv_tabview_add_tab(tabview, LV_SYMBOL_HOME "  Dashboard");
  tab_sensor   = lv_tabview_add_tab(tabview, LV_SYMBOL_WIFI "  Sensors");
  tab_settings = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS "  Settings");

  // ── DASHBOARD TAB ──────────────────────
  lv_obj_set_style_bg_color(tab_dash, lv_color_hex(0x0D1117), 0);
  lv_obj_set_style_pad_all(tab_dash, 8, 0);

  // Time (large)
  lbl_time = lv_label_create(tab_dash);
  lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x58A6FF), 0);
  lv_label_set_text(lbl_time, "--:--:--");
  lv_obj_align(lbl_time, LV_ALIGN_TOP_MID, 0, 0);

  // Date
  lbl_date = lv_label_create(tab_dash);
  lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl_date, lv_color_hex(0x8B949E), 0);
  lv_label_set_text(lbl_date, "--- -- ----");
  lv_obj_align_to(lbl_date, lbl_time, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  // ── Battery card ──
  lv_obj_t* batt_card = lv_obj_create(tab_dash);
  lv_obj_set_size(batt_card, 220, 100);
  lv_obj_align(batt_card, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_color(batt_card, lv_color_hex(0x161B22), 0);
  lv_obj_set_style_border_color(batt_card, lv_color_hex(0x30363D), 0);
  lv_obj_set_style_radius(batt_card, 8, 0);

  lv_obj_t* lbl_batt_title = lv_label_create(batt_card);
  lv_obj_set_style_text_color(lbl_batt_title, lv_color_hex(0x8B949E), 0);
  lv_obj_set_style_text_font(lbl_batt_title, &lv_font_montserrat_12, 0);
  lv_label_set_text(lbl_batt_title, LV_SYMBOL_BATTERY_FULL "  BATTERY");
  lv_obj_align(lbl_batt_title, LV_ALIGN_TOP_LEFT, 4, 4);

  arc_battery = lv_arc_create(batt_card);
  lv_obj_set_size(arc_battery, 70, 70);
  lv_arc_set_rotation(arc_battery, 135);
  lv_arc_set_bg_angles(arc_battery, 0, 270);
  lv_arc_set_value(arc_battery, 0);
  lv_obj_set_style_arc_color(arc_battery, lv_color_hex(0x238636), LV_PART_INDICATOR);
  lv_obj_align(arc_battery, LV_ALIGN_LEFT_MID, 4, 8);
  lv_obj_remove_flag(arc_battery, LV_OBJ_FLAG_CLICKABLE);

  lbl_battery = lv_label_create(batt_card);
  lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lbl_battery, lv_color_white(), 0);
  lv_label_set_text(lbl_battery, "--%");
  lv_obj_align_to(lbl_battery, arc_battery, LV_ALIGN_OUT_RIGHT_MID, 8, -8);

  lbl_charge_status = lv_label_create(batt_card);
  lv_obj_set_style_text_font(lbl_charge_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_charge_status, lv_color_hex(0x8B949E), 0);
  lv_label_set_text(lbl_charge_status, "---");
  lv_obj_align_to(lbl_charge_status, lbl_battery, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

  // ── WiFi card ──
  lv_obj_t* wifi_card = lv_obj_create(tab_dash);
  lv_obj_set_size(wifi_card, 220, 100);
  lv_obj_align(wifi_card, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(wifi_card, lv_color_hex(0x161B22), 0);
  lv_obj_set_style_border_color(wifi_card, lv_color_hex(0x30363D), 0);
  lv_obj_set_style_radius(wifi_card, 8, 0);

  lv_obj_t* lbl_wifi_title = lv_label_create(wifi_card);
  lv_obj_set_style_text_color(lbl_wifi_title, lv_color_hex(0x8B949E), 0);
  lv_obj_set_style_text_font(lbl_wifi_title, &lv_font_montserrat_12, 0);
  lv_label_set_text(lbl_wifi_title, LV_SYMBOL_WIFI "  NETWORK");
  lv_obj_align(lbl_wifi_title, LV_ALIGN_TOP_LEFT, 4, 4);

  lbl_wifi_status = lv_label_create(wifi_card);
  lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_wifi_status, lv_color_hex(0x3FB950), 0);
  lv_label_set_text(lbl_wifi_status, "Connecting...");
  lv_obj_align(lbl_wifi_status, LV_ALIGN_LEFT_MID, 8, -8);

  lbl_ip = lv_label_create(wifi_card);
  lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_ip, lv_color_hex(0x8B949E), 0);
  lv_label_set_text(lbl_ip, "---");
  lv_obj_align_to(lbl_ip, lbl_wifi_status, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

  // ── SENSOR TAB ─────────────────────────
  lv_obj_set_style_bg_color(tab_sensor, lv_color_hex(0x0D1117), 0);
  lv_obj_set_style_pad_all(tab_sensor, 8, 0);

  // Sensor last updated
  lbl_sensor_time = lv_label_create(tab_sensor);
  lv_obj_set_style_text_font(lbl_sensor_time, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_sensor_time, lv_color_hex(0x8B949E), 0);
  lv_label_set_text(lbl_sensor_time, "Last update: --:--");
  lv_obj_align(lbl_sensor_time, LV_ALIGN_TOP_RIGHT, -4, 0);

  // Sensor cards grid
  const char* sensor_labels[] = {
    LV_SYMBOL_UP " TEMP",
    LV_SYMBOL_WIFI " HUMIDITY",
    LV_SYMBOL_CHARGE " PRESSURE",
    LV_SYMBOL_WARNING " AIR QUALITY"
  };
  lv_color_t card_colors[] = {
    lv_color_hex(0xFF7043),
    lv_color_hex(0x42A5F5),
    lv_color_hex(0x66BB6A),
    lv_color_hex(0xAB47BC)
  };
  lv_obj_t** sensor_lbls[] = {&lbl_temp, &lbl_humidity, &lbl_pressure, &lbl_aqi};
  const char* sensor_units[] = {"°C", "%", "hPa", "ppm"};
  const char* sensor_defaults[] = {"--.-", "--.-", "----", "----"};

  for (int i = 0; i < 4; i++) {
    int col = i % 2;
    int row = i / 2;

    lv_obj_t* card = lv_obj_create(tab_sensor);
    lv_obj_set_size(card, 218, 100);
    lv_obj_set_pos(card, col * 228, row * 108 + 20);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_border_color(card, card_colors[i], 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);

    lv_obj_t* lbl_title = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_title, card_colors[i], 0);
    lv_label_set_text(lbl_title, sensor_labels[i]);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 4, 4);

    *sensor_lbls[i] = lv_label_create(card);
    lv_obj_set_style_text_font(*sensor_lbls[i], &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(*sensor_lbls[i], lv_color_white(), 0);
    lv_label_set_text(*sensor_lbls[i], sensor_defaults[i]);
    lv_obj_align(*sensor_lbls[i], LV_ALIGN_LEFT_MID, 4, 8);

    lv_obj_t* lbl_unit = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_unit, lv_color_hex(0x8B949E), 0);
    lv_label_set_text(lbl_unit, sensor_units[i]);
    lv_obj_align_to(lbl_unit, *sensor_lbls[i], LV_ALIGN_OUT_RIGHT_BOTTOM, 4, 0);
  }

  // ── SETTINGS TAB ───────────────────────
  lv_obj_set_style_bg_color(tab_settings, lv_color_hex(0x0D1117), 0);
  lv_obj_set_style_pad_all(tab_settings, 8, 0);

  // Buzzer toggle
  lv_obj_t* lbl_sw_buzz = lv_label_create(tab_settings);
  lv_obj_set_style_text_color(lbl_sw_buzz, lv_color_white(), 0);
  lv_label_set_text(lbl_sw_buzz, LV_SYMBOL_AUDIO " Buzzer");
  lv_obj_align(lbl_sw_buzz, LV_ALIGN_TOP_LEFT, 4, 10);

  sw_buzzer = lv_switch_create(tab_settings);
  lv_obj_align(sw_buzzer, LV_ALIGN_TOP_RIGHT, -4, 6);
  lv_obj_add_state(sw_buzzer, LV_STATE_CHECKED);
  lv_obj_add_event_cb(sw_buzzer, [](lv_event_t* e) {
    buzzerEnabled = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
    if (buzzerEnabled) beepClick();
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  // Brightness slider
  lv_obj_t* lbl_bright = lv_label_create(tab_settings);
  lv_obj_set_style_text_color(lbl_bright, lv_color_white(), 0);
  lv_label_set_text(lbl_bright, LV_SYMBOL_IMAGE " Brightness");
  lv_obj_align(lbl_bright, LV_ALIGN_TOP_LEFT, 4, 50);

  slider_brightness = lv_slider_create(tab_settings);
  lv_obj_set_width(slider_brightness, 200);
  lv_slider_set_range(slider_brightness, 50, 255);
  lv_slider_set_value(slider_brightness, brightness, LV_ANIM_OFF);
  lv_obj_align(slider_brightness, LV_ALIGN_TOP_RIGHT, -4, 54);
  lv_obj_add_event_cb(slider_brightness, [](lv_event_t* e) {
    brightness = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    analogWrite(TFT_LED, brightness);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  // SD info
  lv_obj_t* lbl_sd_title = lv_label_create(tab_settings);
  lv_obj_set_style_text_color(lbl_sd_title, lv_color_white(), 0);
  lv_label_set_text(lbl_sd_title, LV_SYMBOL_SD_CARD " SD Card");
  lv_obj_align(lbl_sd_title, LV_ALIGN_TOP_LEFT, 4, 90);

  lbl_sd_info = lv_label_create(tab_settings);
  lv_obj_set_style_text_color(lbl_sd_info, lv_color_hex(0x8B949E), 0);
  lv_obj_set_style_text_font(lbl_sd_info, &lv_font_montserrat_12, 0);
  lv_label_set_text(lbl_sd_info, "Checking...");
  lv_obj_align(lbl_sd_info, LV_ALIGN_TOP_RIGHT, -4, 94);

  // WiFi reconnect button
  lv_obj_t* btn_reconnect = lv_btn_create(tab_settings);
  lv_obj_set_size(btn_reconnect, 200, 40);
  lv_obj_align(btn_reconnect, LV_ALIGN_BOTTOM_LEFT, 4, -4);
  lv_obj_set_style_bg_color(btn_reconnect, lv_color_hex(0x1F6FEB), 0);
  lv_obj_t* lbl_btn = lv_label_create(btn_reconnect);
  lv_obj_center(lbl_btn);
  lv_label_set_text(lbl_btn, LV_SYMBOL_REFRESH " Reconnect WiFi");
  lv_obj_add_event_cb(btn_reconnect, [](lv_event_t* e) {
    beepClick();
    showNotif("Reconnecting WiFi...", lv_color_hex(0x1F6FEB));
    WiFi.disconnect();
    delay(500);
    initWifi();
  }, LV_EVENT_CLICKED, nullptr);

  // Clear log button
  lv_obj_t* btn_clearlog = lv_btn_create(tab_settings);
  lv_obj_set_size(btn_clearlog, 200, 40);
  lv_obj_align(btn_clearlog, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
  lv_obj_set_style_bg_color(btn_clearlog, lv_color_hex(0xDA3633), 0);
  lv_obj_t* lbl_btn2 = lv_label_create(btn_clearlog);
  lv_obj_center(lbl_btn2);
  lv_label_set_text(lbl_btn2, LV_SYMBOL_TRASH " Clear Log");
  lv_obj_add_event_cb(btn_clearlog, [](lv_event_t* e) {
    beepClick();
    if (sdMounted) {
      SD.remove("/hub_log.csv");
      showNotif("Log cleared!", lv_color_hex(0x238636));
    } else {
      showNotif("No SD card!", lv_color_hex(0xDA3633));
    }
  }, LV_EVENT_CLICKED, nullptr);
}

// ─────────────────────────────────────────
// UPDATE UI
// ─────────────────────────────────────────
void updateClockUI() {
  if (WiFi.status() != WL_CONNECTED) return;

  time_t epoch = timeClient.getEpochTime();
  struct tm* t = localtime(&epoch);

  char timebuf[12], datebuf[20];
  strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);
  strftime(datebuf, sizeof(datebuf), "%a %d %b %Y", t);

  lv_label_set_text(lbl_time, timebuf);
  lv_label_set_text(lbl_date, datebuf);
}

void updateBatteryUI() {
  // Arc color by level
  lv_color_t arc_color;
  if (battery.percent > 50)      arc_color = lv_color_hex(0x238636);
  else if (battery.percent > 20) arc_color = lv_color_hex(0xD29922);
  else                            arc_color = lv_color_hex(0xDA3633);

  lv_obj_set_style_arc_color(arc_battery, arc_color, LV_PART_INDICATOR);
  lv_arc_set_value(arc_battery, battery.percent);

  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", battery.percent);
  lv_label_set_text(lbl_battery, buf);

  // Charge status
  if (battery.full)          lv_label_set_text(lbl_charge_status, LV_SYMBOL_OK " Full");
  else if (battery.charging) lv_label_set_text(lbl_charge_status, LV_SYMBOL_CHARGE " Charging");
  else if (battery.usbPower) lv_label_set_text(lbl_charge_status, LV_SYMBOL_USB " USB Power");
  else                        lv_label_set_text(lbl_charge_status, LV_SYMBOL_BATTERY_EMPTY " Battery");

  // Low battery warning
  if (battery.percent <= BATT_LOW_PCT && !battery.charging) {
    showNotif("Low battery!", lv_color_hex(0xDA3633));
    beep(500, 200);
    delay(100);
    beep(500, 200);
  }
}

void updateWifiUI() {
  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text(lbl_wifi_status, LV_SYMBOL_WIFI " Connected");
    lv_obj_set_style_text_color(lbl_wifi_status, lv_color_hex(0x3FB950), 0);
    lv_label_set_text(lbl_ip, WiFi.localIP().toString().c_str());
  } else {
    lv_label_set_text(lbl_wifi_status, LV_SYMBOL_CLOSE " Disconnected");
    lv_obj_set_style_text_color(lbl_wifi_status, lv_color_hex(0xDA3633), 0);
    lv_label_set_text(lbl_ip, "---");
  }
}

void updateSensorUI() {
  if (!sensor.fresh) return;

  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f", sensor.temp);
  lv_label_set_text(lbl_temp, buf);

  snprintf(buf, sizeof(buf), "%.1f", sensor.humidity);
  lv_label_set_text(lbl_humidity, buf);

  snprintf(buf, sizeof(buf), "%.0f", sensor.pressure);
  lv_label_set_text(lbl_pressure, buf);

  snprintf(buf, sizeof(buf), "%.0f", sensor.aqi);
  lv_label_set_text(lbl_aqi, buf);

  char timebuf[32];
  snprintf(timebuf, sizeof(timebuf), "Last update: %s", sensor.time);
  lv_label_set_text(lbl_sensor_time, timebuf);

  sensor.fresh = false;
}

void updateSettingsUI() {
  lv_label_set_text(lbl_sd_info, getSDInfo().c_str());
}

// ─────────────────────────────────────────
// SIMULATE SENSOR DATA
// Replace this with actual LoRa parsing later
// ─────────────────────────────────────────
void checkSensorData() {
  // TODO: replace with LoRa receive code
  // Simulated data for now
  sensor.temp     = 24.5 + random(-20, 20) * 0.1;
  sensor.humidity = 60.0 + random(-50, 50) * 0.1;
  sensor.pressure = 1013.0 + random(-50, 50) * 0.1;
  sensor.aqi      = 400 + random(0, 200);
  sensor.fresh    = true;

  // Get current time string
  time_t epoch = timeClient.getEpochTime();
  struct tm* t = localtime(&epoch);
  strftime(sensor.time, sizeof(sensor.time), "%H:%M:%S", t);

  // Log to SD
  if (sdMounted) {
    char logbuf[128];
    snprintf(logbuf, sizeof(logbuf),
      "%s,%.1f,%.1f,%.0f,%.0f",
      sensor.time, sensor.temp, sensor.humidity,
      sensor.pressure, sensor.aqi);
    logToSD(logbuf);
  }
}

// ─────────────────────────────────────────
// PHYSICAL BUTTONS
// ─────────────────────────────────────────
void handleButtons() {
  static bool backPrev   = HIGH;
  static bool homePrev   = HIGH;
  static bool customPrev = HIGH;

  bool back   = digitalRead(BTN_BACK);
  bool home   = digitalRead(BTN_HOME);
  bool custom = digitalRead(BTN_CUSTOM);

  if (back == LOW && backPrev == HIGH) {
    beepClick();
    // Navigate to previous tab
    uint16_t cur = lv_tabview_get_tab_act(tabview);
    if (cur > 0) lv_tabview_set_act(tabview, cur - 1, LV_ANIM_ON);
  }
  if (home == LOW && homePrev == HIGH) {
    beepClick();
    lv_tabview_set_act(tabview, 0, LV_ANIM_ON);
  }
  if (custom == LOW && customPrev == HIGH) {
    beepClick();
    uint16_t cur = lv_tabview_get_tab_act(tabview);
    if (cur < 2) lv_tabview_set_act(tabview, cur + 1, LV_ANIM_ON);
  }

  backPrev   = back;
  homePrev   = home;
  customPrev = custom;
}

// ─────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32-S3 Hub Starting ===");

  // Pin init
  pinMode(BTN_BACK,   INPUT_PULLUP);
  pinMode(BTN_HOME,   INPUT_PULLUP);
  pinMode(BTN_CUSTOM, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STAT1_PIN,  INPUT_PULLUP);
  pinMode(STAT2_PIN,  INPUT_PULLUP);
  pinMode(PG_PIN,     INPUT_PULLUP);
  pinMode(SD_CD,      INPUT_PULLUP);
  pinMode(TFT_LED,    OUTPUT);
  analogWrite(TFT_LED, brightness);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Display init
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // LVGL init
  lv_init();

  buf1 = (lv_color_t*)heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  buf2 = (lv_color_t*)heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUF_SIZE);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res  = 480;
  disp_drv.ver_res  = 320;
  disp_drv.flush_cb = lvgl_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type    = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_touch;
  lv_indev_drv_register(&indev_drv);

  // Splash screen via LVGL
  lv_obj_t* splash = lv_obj_create(lv_scr_act());
  lv_obj_set_size(splash, 480, 320);
  lv_obj_set_style_bg_color(splash, lv_color_hex(0x0D1117), 0);
  lv_obj_set_style_border_width(splash, 0, 0);

  lv_obj_t* lbl_splash = lv_label_create(splash);
  lv_obj_set_style_text_font(lbl_splash, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(lbl_splash, lv_color_hex(0x58A6FF), 0);
  lv_label_set_text(lbl_splash, "ESP32-S3 HUB");
  lv_obj_align(lbl_splash, LV_ALIGN_CENTER, 0, -20);

  lv_obj_t* lbl_ver = lv_label_create(splash);
  lv_obj_set_style_text_color(lbl_ver, lv_color_hex(0x8B949E), 0);
  lv_label_set_text(lbl_ver, "Initialising...");
  lv_obj_align(lbl_ver, LV_ALIGN_CENTER, 0, 20);

  lv_timer_handler();
  beep(1000, 100); delay(100); beep(1500, 100);

  // SD init
  lv_label_set_text(lbl_ver, "Mounting SD...");
  lv_timer_handler();
  sdMounted = initSD();
  if (sdMounted) {
    Serial.println("SD mounted: " + getSDInfo());
    // Write CSV header if new file
    if (!SD.exists("/hub_log.csv")) {
      logToSD("time,temp,humidity,pressure,aqi");
    }
  }

  // WiFi
  lv_label_set_text(lbl_ver, "Connecting WiFi...");
  lv_timer_handler();
  initWifi();

  // Build UI
  lv_label_set_text(lbl_ver, "Building UI...");
  lv_timer_handler();
  delay(300);

  lv_obj_del(splash);
  buildUI();

  // Initial reads
  updateBattery();
  updateBatteryUI();
  updateWifiUI();
  updateSettingsUI();

  beepSuccess();
  Serial.println("=== Hub Ready ===");
}

// ─────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────
void loop() {
  lv_timer_handler();

  unsigned long now = millis();

  // Clock — every second
  if (now - lastUIUpdate >= 1000) {
    if (WiFi.status() == WL_CONNECTED) timeClient.update();
    updateClockUI();
    updateWifiUI();
    lastUIUpdate = now;
  }

  // Battery — every 10 seconds
  if (now - lastBattUpdate >= 10000) {
    updateBattery();
    updateBatteryUI();
    lastBattUpdate = now;
  }

  // Sensor data — every 30 seconds
  if (now - lastSensorCheck >= 30000) {
    checkSensorData();
    updateSensorUI();
    updateSettingsUI();
    lastSensorCheck = now;
  }

  // Physical buttons
  handleButtons();

  delay(5);
}
