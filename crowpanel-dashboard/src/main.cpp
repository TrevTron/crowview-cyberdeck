/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  CALPURNICA CROWPANEL — Main Firmware                       ║
 * ║  ESP32-S3 + LovyanGFX + LVGL 8.3                          ║
 * ║  Polls Nova HTTP endpoint for RF telemetry data             ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <lvgl.h>
#include "display_driver.h"

// ── Configuration ───────────────────────────────────────────────
#define WIFI_SSID       "Pump House"
#define WIFI_PASS       "9517641004"
#define NOVA_IP         "192.168.5.21"
#define NOVA_STATS_PORT 8088
#define POLL_INTERVAL   2000
#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   480

// ── LVGL Buffers ────────────────────────────────────────────────
static lv_disp_draw_buf_t draw_buf;

// ── Color Palette ───────────────────────────────────────────────
#define COLOR_BG         lv_color_hex(0x0A0E14)
#define COLOR_PANEL_BG   lv_color_hex(0x111820)
#define COLOR_BORDER     lv_color_hex(0x1A2530)
#define COLOR_CYAN       lv_color_hex(0x00E5FF)
#define COLOR_GREEN      lv_color_hex(0x00E676)
#define COLOR_AMBER      lv_color_hex(0xFFAB00)
#define COLOR_RED        lv_color_hex(0xFF1744)
#define COLOR_WHITE      lv_color_hex(0xE0E0E0)
#define COLOR_DIM        lv_color_hex(0x4A5568)
#define COLOR_DARK       lv_color_hex(0x1A1A2E)

// ── LVGL Objects ────────────────────────────────────────────────
static lv_obj_t *lbl_title, *lbl_status, *lbl_time;
static lv_obj_t *lbl_band, *lbl_freq, *lbl_snr_val, *bar_snr;
static lv_obj_t *lbl_peak, *lbl_floor, *lbl_scans, *lbl_anomalies;
static lv_obj_t *lbl_class, *lbl_conf_val, *bar_confidence, *lbl_class_badge;
static lv_obj_t *lbl_cpu_val, *bar_cpu, *lbl_ram_val, *bar_ram;
static lv_obj_t *lbl_temp_val, *arc_temp, *lbl_uptime;
static lv_obj_t *chart_snr;
static lv_chart_series_t *ser_snr;
static lv_obj_t *lbl_wifi_status, *lbl_last_update;

// ── Styles ──────────────────────────────────────────────────────
static lv_style_t style_panel, style_title, style_value_large;
static lv_style_t style_value_small, style_label_dim, style_bar, style_bar_ind;

// ── State ───────────────────────────────────────────────────────
static unsigned long last_poll = 0;
static bool wifi_connected = false;


// Flush and touch callbacks are in display_driver.h:
//   display_flush_cb() and touch_read_cb()


// ════════════════════════════════════════════════════════════════
// Style initialization
// ════════════════════════════════════════════════════════════════
void init_styles() {
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, COLOR_PANEL_BG);
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_border_color(&style_panel, COLOR_BORDER);
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_radius(&style_panel, 8);
    lv_style_set_pad_all(&style_panel, 10);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, COLOR_CYAN);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_14);

    lv_style_init(&style_value_large);
    lv_style_set_text_color(&style_value_large, COLOR_WHITE);
    lv_style_set_text_font(&style_value_large, &lv_font_montserrat_28);

    lv_style_init(&style_value_small);
    lv_style_set_text_color(&style_value_small, COLOR_WHITE);
    lv_style_set_text_font(&style_value_small, &lv_font_montserrat_14);

    lv_style_init(&style_label_dim);
    lv_style_set_text_color(&style_label_dim, COLOR_DIM);
    lv_style_set_text_font(&style_label_dim, &lv_font_montserrat_12);

    lv_style_init(&style_bar);
    lv_style_set_bg_color(&style_bar, COLOR_DARK);
    lv_style_set_bg_opa(&style_bar, LV_OPA_COVER);
    lv_style_set_radius(&style_bar, 4);

    lv_style_init(&style_bar_ind);
    lv_style_set_radius(&style_bar_ind, 4);
}


// ════════════════════════════════════════════════════════════════
// UI Helpers
// ════════════════════════════════════════════════════════════════
lv_obj_t* create_panel(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                        lv_coord_t w, lv_coord_t h, const char *title) {
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_add_style(panel, &style_panel, 0);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(panel);
    lv_obj_add_style(lbl, &style_title, 0);
    lv_label_set_text(lbl, title);
    lv_obj_set_pos(lbl, 5, 0);

    lv_obj_t *line = lv_obj_create(panel);
    lv_obj_set_size(line, w - 30, 1);
    lv_obj_set_pos(line, 5, 20);
    lv_obj_set_style_bg_color(line, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    return panel;
}

lv_obj_t* create_bar(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                      lv_coord_t w, lv_color_t color) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_size(bar, w, 12);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_add_style(bar, &style_bar, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_add_style(bar, &style_bar_ind, LV_PART_INDICATOR);
    return bar;
}


// ════════════════════════════════════════════════════════════════
// Build the UI
// ════════════════════════════════════════════════════════════════
void ui_init() {
    init_styles();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Header Bar ──────────────────────────────────────────
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, SCREEN_WIDTH, 44);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lbl_title = lv_label_create(header);
    lv_obj_set_style_text_color(lbl_title, COLOR_CYAN, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_title, LV_SYMBOL_EYE_OPEN " CALPURNICA");
    lv_obj_set_pos(lbl_title, 15, 10);

    lbl_status = lv_label_create(header);
    lv_obj_set_style_text_color(lbl_status, COLOR_AMBER, 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_status, LV_SYMBOL_WARNING " CONNECTING");
    lv_obj_set_pos(lbl_status, 300, 13);

    lbl_time = lv_label_create(header);
    lv_obj_set_style_text_color(lbl_time, COLOR_DIM, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_time, "--:--:--");
    lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, -15, 13);

    // ── Scanner Panel (top-left) ────────────────────────────
    lv_obj_t *p_scan = create_panel(scr, 8, 50, 385, 200,
                                     LV_SYMBOL_WIFI " RF SCANNER");

    lv_obj_t *tmp = lv_label_create(p_scan);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "ACTIVE BAND");
    lv_obj_set_pos(tmp, 5, 28);

    lbl_band = lv_label_create(p_scan);
    lv_obj_add_style(lbl_band, &style_value_large, 0);
    lv_label_set_text(lbl_band, "STANDBY");
    lv_obj_set_pos(lbl_band, 5, 42);

    lbl_freq = lv_label_create(p_scan);
    lv_obj_set_style_text_color(lbl_freq, COLOR_CYAN, 0);
    lv_obj_set_style_text_font(lbl_freq, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl_freq, "0.000 MHz");
    lv_obj_set_pos(lbl_freq, 5, 74);

    tmp = lv_label_create(p_scan);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "SNR");
    lv_obj_set_pos(tmp, 5, 100);

    bar_snr = create_bar(p_scan, 35, 102, 250, COLOR_GREEN);
    lv_bar_set_range(bar_snr, 0, 50);

    lbl_snr_val = lv_label_create(p_scan);
    lv_obj_add_style(lbl_snr_val, &style_value_small, 0);
    lv_label_set_text(lbl_snr_val, "0.0 dB");
    lv_obj_set_pos(lbl_snr_val, 295, 98);

    lbl_peak = lv_label_create(p_scan);
    lv_obj_add_style(lbl_peak, &style_label_dim, 0);
    lv_label_set_text(lbl_peak, "Peak: --- dBFS");
    lv_obj_set_pos(lbl_peak, 5, 125);

    lbl_floor = lv_label_create(p_scan);
    lv_obj_add_style(lbl_floor, &style_label_dim, 0);
    lv_label_set_text(lbl_floor, "Floor: --- dBFS");
    lv_obj_set_pos(lbl_floor, 170, 125);

    lbl_scans = lv_label_create(p_scan);
    lv_obj_set_style_text_color(lbl_scans, COLOR_CYAN, 0);
    lv_obj_set_style_text_font(lbl_scans, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_scans, "SCANS: 0");
    lv_obj_set_pos(lbl_scans, 5, 150);

    lbl_anomalies = lv_label_create(p_scan);
    lv_obj_set_style_text_color(lbl_anomalies, COLOR_AMBER, 0);
    lv_obj_set_style_text_font(lbl_anomalies, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_anomalies, "ANOMALIES: 0");
    lv_obj_set_pos(lbl_anomalies, 200, 150);

    // ── Classifier Panel (top-right) ────────────────────────
    lv_obj_t *p_class = create_panel(scr, 403, 50, 389, 200,
                                      LV_SYMBOL_EYE_OPEN " ML CLASSIFIER");

    tmp = lv_label_create(p_class);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "SIGNAL TYPE");
    lv_obj_set_pos(tmp, 5, 28);

    lbl_class_badge = lv_obj_create(p_class);
    lv_obj_set_size(lbl_class_badge, 360, 50);
    lv_obj_set_pos(lbl_class_badge, 5, 44);
    lv_obj_set_style_bg_color(lbl_class_badge, lv_color_hex(0x37474F), 0);
    lv_obj_set_style_bg_opa(lbl_class_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lbl_class_badge, 6, 0);
    lv_obj_set_style_border_width(lbl_class_badge, 0, 0);
    lv_obj_clear_flag(lbl_class_badge, LV_OBJ_FLAG_SCROLLABLE);

    lbl_class = lv_label_create(lbl_class_badge);
    lv_obj_set_style_text_color(lbl_class, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(lbl_class, &lv_font_montserrat_28, 0);
    lv_label_set_text(lbl_class, "STANDBY");
    lv_obj_center(lbl_class);

    tmp = lv_label_create(p_class);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "CONFIDENCE");
    lv_obj_set_pos(tmp, 5, 105);

    bar_confidence = create_bar(p_class, 5, 122, 290, COLOR_GREEN);

    lbl_conf_val = lv_label_create(p_class);
    lv_obj_set_style_text_color(lbl_conf_val, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(lbl_conf_val, &lv_font_montserrat_28, 0);
    lv_label_set_text(lbl_conf_val, "0%");
    lv_obj_set_pos(lbl_conf_val, 305, 112);

    tmp = lv_label_create(p_class);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "RTL-ML v2  |  RandomForest  |  8 classes  |  96.9% acc");
    lv_obj_set_pos(tmp, 5, 155);

    // ── System Panel (bottom-left) ──────────────────────────
    lv_obj_t *p_sys = create_panel(scr, 8, 260, 385, 175,
                                    LV_SYMBOL_CHARGE " SYSTEM TELEMETRY");

    tmp = lv_label_create(p_sys);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "CPU");
    lv_obj_set_pos(tmp, 5, 30);

    bar_cpu = create_bar(p_sys, 40, 32, 240, COLOR_CYAN);

    lbl_cpu_val = lv_label_create(p_sys);
    lv_obj_add_style(lbl_cpu_val, &style_value_small, 0);
    lv_label_set_text(lbl_cpu_val, "0%");
    lv_obj_set_pos(lbl_cpu_val, 290, 28);

    tmp = lv_label_create(p_sys);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "RAM");
    lv_obj_set_pos(tmp, 5, 55);

    bar_ram = create_bar(p_sys, 40, 57, 240, COLOR_GREEN);

    lbl_ram_val = lv_label_create(p_sys);
    lv_obj_add_style(lbl_ram_val, &style_value_small, 0);
    lv_label_set_text(lbl_ram_val, "0%");
    lv_obj_set_pos(lbl_ram_val, 290, 53);

    arc_temp = lv_arc_create(p_sys);
    lv_obj_set_size(arc_temp, 80, 80);
    lv_obj_set_pos(arc_temp, 5, 75);
    lv_arc_set_range(arc_temp, 68, 185);
    lv_arc_set_value(arc_temp, 100);
    lv_arc_set_bg_angles(arc_temp, 135, 405);
    lv_obj_set_style_arc_color(arc_temp, COLOR_DARK, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_temp, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_temp, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_temp, 8, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_temp, NULL, LV_PART_KNOB);

    lbl_temp_val = lv_label_create(p_sys);
    lv_obj_set_style_text_color(lbl_temp_val, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(lbl_temp_val, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_temp_val, "0\xC2\xB0" "F");
    lv_obj_set_pos(lbl_temp_val, 20, 105);

    tmp = lv_label_create(p_sys);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "UPTIME");
    lv_obj_set_pos(tmp, 110, 80);

    lbl_uptime = lv_label_create(p_sys);
    lv_obj_add_style(lbl_uptime, &style_value_small, 0);
    lv_label_set_text(lbl_uptime, "0:00:00");
    lv_obj_set_pos(lbl_uptime, 110, 98);

    tmp = lv_label_create(p_sys);
    lv_obj_add_style(tmp, &style_label_dim, 0);
    lv_label_set_text(tmp, "RK3588S  |  16GB  |  Debian  |  RTL-SDR V4");
    lv_obj_set_pos(tmp, 110, 120);

    // ── SNR History Chart (bottom-right) ────────────────────
    lv_obj_t *p_chart = create_panel(scr, 403, 260, 389, 175,
                                      LV_SYMBOL_REFRESH " SNR HISTORY");

    chart_snr = lv_chart_create(p_chart);
    lv_obj_set_size(chart_snr, 355, 130);
    lv_obj_set_pos(chart_snr, 5, 28);
    lv_chart_set_type(chart_snr, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_snr, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    lv_chart_set_point_count(chart_snr, 30);
    lv_chart_set_div_line_count(chart_snr, 4, 0);

    lv_obj_set_style_bg_color(chart_snr, COLOR_DARK, 0);
    lv_obj_set_style_bg_opa(chart_snr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chart_snr, COLOR_BORDER, 0);
    lv_obj_set_style_line_color(chart_snr, lv_color_hex(0x1A2530), LV_PART_MAIN);

    ser_snr = lv_chart_add_series(chart_snr, COLOR_CYAN, LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(chart_snr, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_snr, 0, LV_PART_INDICATOR);

    for (int i = 0; i < 30; i++) {
        lv_chart_set_next_value(chart_snr, ser_snr, 0);
    }

    // ── Footer Bar ──────────────────────────────────────────
    lv_obj_t *footer = lv_obj_create(scr);
    lv_obj_set_size(footer, SCREEN_WIDTH, 36);
    lv_obj_set_pos(footer, 0, SCREEN_HEIGHT - 36);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lbl_wifi_status = lv_label_create(footer);
    lv_obj_set_style_text_color(lbl_wifi_status, COLOR_DIM, 0);
    lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_wifi_status, LV_SYMBOL_WIFI " Connecting...");
    lv_obj_set_pos(lbl_wifi_status, 15, 10);

    lbl_last_update = lv_label_create(footer);
    lv_obj_set_style_text_color(lbl_last_update, COLOR_DIM, 0);
    lv_obj_set_style_text_font(lbl_last_update, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_last_update, "Calpurnica RF Intelligence  |  Nova + CrowPanel");
    lv_obj_align(lbl_last_update, LV_ALIGN_TOP_RIGHT, -15, 10);
}


// ════════════════════════════════════════════════════════════════
// Update UI from polled data
// ════════════════════════════════════════════════════════════════
void ui_update_from_json(JsonDocument &doc) {
    char buf[64];

    bool sdr_active = doc["sdr_active"] | false;

    if (sdr_active) {
        lv_label_set_text(lbl_status, LV_SYMBOL_OK " LIVE");
        lv_obj_set_style_text_color(lbl_status, COLOR_GREEN, 0);
    } else {
        lv_label_set_text(lbl_status, LV_SYMBOL_WARNING " DEMO");
        lv_obj_set_style_text_color(lbl_status, COLOR_AMBER, 0);
    }

    const char *band = doc["band"] | "---";
    lv_label_set_text(lbl_band, band);

    float freq = doc["freq_mhz"] | 0.0f;
    snprintf(buf, sizeof(buf), "%.3f MHz", freq);
    lv_label_set_text(lbl_freq, buf);

    float snr = doc["snr"] | 0.0f;
    lv_bar_set_value(bar_snr, (int)snr, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%.1f dB", snr);
    lv_label_set_text(lbl_snr_val, buf);

    lv_color_t snr_color = snr > 20 ? COLOR_GREEN : snr > 10 ? COLOR_AMBER : COLOR_RED;
    lv_obj_set_style_bg_color(bar_snr, snr_color, LV_PART_INDICATOR);

    // SNR chart
    lv_chart_set_next_value(chart_snr, ser_snr, (lv_coord_t)snr);
    lv_chart_refresh(chart_snr);

    float peak = doc["peak_power"] | 0.0f;
    snprintf(buf, sizeof(buf), "Peak: %.1f dBFS", peak);
    lv_label_set_text(lbl_peak, buf);

    float floor_db = doc["noise_floor"] | 0.0f;
    snprintf(buf, sizeof(buf), "Floor: %.1f dBFS", floor_db);
    lv_label_set_text(lbl_floor, buf);

    int scans = doc["scan_count"] | 0;
    snprintf(buf, sizeof(buf), "SCANS: %d", scans);
    lv_label_set_text(lbl_scans, buf);

    int anomalies = doc["anomaly_count"] | 0;
    snprintf(buf, sizeof(buf), "ANOMALIES: %d", anomalies);
    lv_label_set_text(lbl_anomalies, buf);

    // Classifier
    const char *cls = doc["classification"] | "---";
    lv_label_set_text(lbl_class, cls);
    lv_obj_center(lbl_class);

    float conf = doc["confidence"] | 0.0f;
    lv_bar_set_value(bar_confidence, (int)conf, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%.0f%%", conf);
    lv_label_set_text(lbl_conf_val, buf);

    lv_color_t conf_color = conf > 85 ? COLOR_GREEN : conf > 60 ? COLOR_AMBER : COLOR_RED;
    lv_obj_set_style_bg_color(bar_confidence, conf_color, LV_PART_INDICATOR);

    bool is_signal = strcmp(cls, "noise") != 0 && strcmp(cls, "---") != 0;
    lv_obj_set_style_bg_color(lbl_class_badge,
        is_signal ? lv_color_hex(0x1B5E20) : lv_color_hex(0x37474F), 0);

    // System
    float cpu = doc["cpu"] | 0.0f;
    lv_bar_set_value(bar_cpu, (int)cpu, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%.0f%%", cpu);
    lv_label_set_text(lbl_cpu_val, buf);

    lv_color_t cpu_color = cpu < 60 ? COLOR_CYAN : cpu < 85 ? COLOR_AMBER : COLOR_RED;
    lv_obj_set_style_bg_color(bar_cpu, cpu_color, LV_PART_INDICATOR);

    float ram = doc["ram"] | 0.0f;
    lv_bar_set_value(bar_ram, (int)ram, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%.0f%%", ram);
    lv_label_set_text(lbl_ram_val, buf);

    float temp_c = doc["temp"] | 0.0f;
    float temp = temp_c * 9.0f / 5.0f + 32.0f;
    lv_arc_set_value(arc_temp, (int)temp);
    snprintf(buf, sizeof(buf), "%.0f\xC2\xB0" "F", temp);
    lv_label_set_text(lbl_temp_val, buf);

    lv_color_t temp_color = temp < 131 ? COLOR_GREEN : temp < 158 ? COLOR_AMBER : COLOR_RED;
    lv_obj_set_style_arc_color(arc_temp, temp_color, LV_PART_INDICATOR);

    const char *upt = doc["uptime"] | "0:00:00";
    lv_label_set_text(lbl_uptime, upt);
}


// ════════════════════════════════════════════════════════════════
// Poll Nova for stats
// ════════════════════════════════════════════════════════════════
void poll_nova() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d/stats", NOVA_IP, NOVA_STATS_PORT);
    http.begin(url);
    http.setTimeout(3000);

    int code = http.GET();
    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            ui_update_from_json(doc);
            lv_label_set_text(lbl_wifi_status,
                LV_SYMBOL_WIFI " Connected to Nova");
            lv_obj_set_style_text_color(lbl_wifi_status, COLOR_GREEN, 0);
        }
    } else {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING " Nova: HTTP %d", code);
        lv_label_set_text(lbl_wifi_status, buf);
        lv_obj_set_style_text_color(lbl_wifi_status, COLOR_AMBER, 0);
    }
    http.end();
}


// ════════════════════════════════════════════════════════════════
// WiFi connection
// ════════════════════════════════════════════════════════════════
void wifi_connect() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to %s", WIFI_SSID);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(250);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi failed — running in offline mode");
    }
}


// ════════════════════════════════════════════════════════════════
// Setup
// ════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    Serial.println("\n[Calpurnica CrowPanel]");

    // Init display (LovyanGFX)
    if (!display_init()) {
        Serial.println("FATAL: Display init failed!");
        while(1) delay(1000);
    }

    // Init LVGL
    lv_init();

    // Draw buffers — static arrays (LovyanGFX handles PSRAM framebuffer internally)
    static lv_color_t disp_draw_buf1[SCREEN_WIDTH * SCREEN_HEIGHT / 10];
    static lv_color_t disp_draw_buf2[SCREEN_WIDTH * SCREEN_HEIGHT / 10];
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf1, disp_draw_buf2, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

    // Register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;  // Required for RGB panel DMA
    lv_disp_drv_register(&disp_drv);

    // Register touch input
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    // Build UI
    ui_init();

    // Connect WiFi
    wifi_connect();

    if (wifi_connected) {
        char buf[48];
        snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %s  |  %s",
                 WIFI_SSID, WiFi.localIP().toString().c_str());
        lv_label_set_text(lbl_wifi_status, buf);
        lv_obj_set_style_text_color(lbl_wifi_status, COLOR_GREEN, 0);
    }
}


// ════════════════════════════════════════════════════════════════
// Loop
// ════════════════════════════════════════════════════════════════
void loop() {
    lv_timer_handler();

    unsigned long now = millis();
    if (now - last_poll >= POLL_INTERVAL) {
        last_poll = now;

        // Update time display
        unsigned long sec = now / 1000;
        char tbuf[16];
        snprintf(tbuf, sizeof(tbuf), "%lu:%02lu:%02lu",
                 sec / 3600, (sec % 3600) / 60, sec % 60);
        lv_label_set_text(lbl_time, tbuf);

        // Poll Nova
        if (wifi_connected) {
            poll_nova();
        }

        // WiFi reconnect
        if (WiFi.status() != WL_CONNECTED && wifi_connected) {
            wifi_connected = false;
            lv_label_set_text(lbl_wifi_status,
                LV_SYMBOL_CLOSE " WiFi disconnected — reconnecting...");
            lv_obj_set_style_text_color(lbl_wifi_status, COLOR_RED, 0);
            WiFi.reconnect();
        } else if (WiFi.status() == WL_CONNECTED && !wifi_connected) {
            wifi_connected = true;
        }
    }

    delay(5);
}
