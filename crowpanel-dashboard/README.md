# CrowPanel Dashboard Firmware

ESP32-S3 firmware for the CrowPanel 7.0" that displays real-time RF scanning data from a host SBC.

## Features

- LVGL 8.3 UI with dark cyberpunk theme
- Real-time SNR bar and 30-point scrolling history chart
- ML signal classification display with confidence bar
- System telemetry gauges (CPU, RAM, temperature)
- Polls stats endpoint every 2 seconds over WiFi

## Build

Requires [PlatformIO](https://platformio.org/).

```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

## Configuration

Edit `src/main.cpp`:

```cpp
#define WIFI_SSID       "YourNetwork"
#define WIFI_PASS       "YourPassword"
#define NOVA_HOST       "192.168.x.x"
#define NOVA_STATS_PORT  8088
```

## Files

- `src/main.cpp` — Full LVGL UI, HTTP polling, JSON parsing
- `src/display_driver.h` — LovyanGFX driver for CrowPanel 7" V3.0 RGB LCD
- `platformio.ini` — Build config (ESP32-S3, 4MB flash, espressif32@6.5.0)
