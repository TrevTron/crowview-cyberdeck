#pragma once

// ════════════════════════════════════════════════════════════════
// CrowPanel 7.0" V3.0 Display Driver — LovyanGFX
// Based on Elecrow's official reference code
// ════════════════════════════════════════════════════════════════

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>

#define LCD_WIDTH   800
#define LCD_HEIGHT  480

// GT911 touch (via PCA9557 reset sequence)
#define TOUCH_SDA   19
#define TOUCH_SCL   20

class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Bus_RGB      _bus_instance;
    lgfx::Panel_RGB    _panel_instance;
    lgfx::Light_PWM    _light_instance;
    lgfx::Touch_GT911  _touch_instance;

    LGFX(void) {
        // Panel config
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = LCD_WIDTH;
            cfg.memory_height = LCD_HEIGHT;
            cfg.panel_width   = LCD_WIDTH;
            cfg.panel_height  = LCD_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }

        // Bus config — RGB parallel
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            // Data pins (16-bit RGB565)
            cfg.pin_d0  = GPIO_NUM_15;  // B0
            cfg.pin_d1  = GPIO_NUM_7;   // B1
            cfg.pin_d2  = GPIO_NUM_6;   // B2
            cfg.pin_d3  = GPIO_NUM_5;   // B3
            cfg.pin_d4  = GPIO_NUM_4;   // B4
            cfg.pin_d5  = GPIO_NUM_9;   // G0
            cfg.pin_d6  = GPIO_NUM_46;  // G1
            cfg.pin_d7  = GPIO_NUM_3;   // G2
            cfg.pin_d8  = GPIO_NUM_8;   // G3
            cfg.pin_d9  = GPIO_NUM_16;  // G4
            cfg.pin_d10 = GPIO_NUM_1;   // G5
            cfg.pin_d11 = GPIO_NUM_14;  // R0
            cfg.pin_d12 = GPIO_NUM_21;  // R1
            cfg.pin_d13 = GPIO_NUM_47;  // R2
            cfg.pin_d14 = GPIO_NUM_48;  // R3
            cfg.pin_d15 = GPIO_NUM_45;  // R4

            // Sync pins
            cfg.pin_henable = GPIO_NUM_41;  // DE
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_0;
            cfg.freq_write  = 12000000;     // 12MHz — Elecrow reference

            // Timing
            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48;
            cfg.hsync_back_porch  = 40;

            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 31;
            cfg.vsync_back_porch  = 13;

            cfg.pclk_active_neg = 1;
            cfg.de_idle_high    = 0;
            cfg.pclk_idle_high  = 0;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Backlight PWM
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = GPIO_NUM_2;
            _light_instance.config(cfg);
            _panel_instance.light(&_light_instance);
        }

        // GT911 Touch
        {
            auto cfg = _touch_instance.config();
            cfg.x_min      = 0;
            cfg.x_max      = 799;
            cfg.y_min      = 0;
            cfg.y_max      = 479;
            cfg.pin_int    = -1;
            cfg.pin_rst    = -1;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;
            cfg.i2c_port   = I2C_NUM_1;
            cfg.pin_sda    = GPIO_NUM_19;
            cfg.pin_scl    = GPIO_NUM_20;
            cfg.freq       = 400000;
            cfg.i2c_addr   = 0x14;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX tft;

// V3.0 board GPIO init + PCA9557 touch reset
void crowpanel_gpio_init() {
    // Control pins — must be LOW for V3.0 board
    pinMode(38, OUTPUT); digitalWrite(38, LOW);
    pinMode(17, OUTPUT); digitalWrite(17, LOW);
    pinMode(18, OUTPUT); digitalWrite(18, LOW);
    pinMode(42, OUTPUT); digitalWrite(42, LOW);

    // PCA9557 touch reset sequence via I2C
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    // PCA9557 address 0x18, output register 0x01, config register 0x03
    // Set IO0 and IO1 as outputs (config = 0xFC)
    Wire.beginTransmission(0x18);
    Wire.write(0x03);  // config register
    Wire.write(0xFC);  // IO0, IO1 as output
    Wire.endTransmission();

    // Set IO0=LOW, IO1=LOW
    Wire.beginTransmission(0x18);
    Wire.write(0x01);  // output register
    Wire.write(0x00);
    Wire.endTransmission();
    delay(20);

    // Set IO0=HIGH (release GT911 reset)
    Wire.beginTransmission(0x18);
    Wire.write(0x01);
    Wire.write(0x01);  // IO0=HIGH
    Wire.endTransmission();
    delay(100);

    // Set IO1 as input (INT pin)
    Wire.beginTransmission(0x18);
    Wire.write(0x03);
    Wire.write(0xFE);  // IO0 output, IO1 input
    Wire.endTransmission();

    Serial.println("PCA9557 touch reset done");
}

// LVGL flush callback — DMA push
void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t*)&color_p->full);
    lv_disp_flush_ready(disp);
}

// LVGL touch callback
void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = tx;
        data->point.y = ty;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

bool display_init() {
    crowpanel_gpio_init();
    tft.begin();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    delay(200);
    Serial.println("LovyanGFX display initialized OK");
    return true;
}
