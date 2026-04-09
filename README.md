# Raspberry Pi Laptop Cyberdeck — Multi-Display SDR Dashboard

**Turn a Raspberry Pi 5 (or any Linux SBC) into a portable RF monitoring station with a [CrowPanel 7" ESP32 HMI display](https://www.elecrow.com/crowpanel-7-0-esp32-hmi-display.html) and [CrowView Note](https://www.elecrow.com/crowview-note-15-6-all-in-one-portable-monitor-phone-to-laptop-device.html?utm_source=Trevor&utm_medium=social&utm_campaign=ABCD&idd=5) laptop monitor.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Python 3.11+](https://img.shields.io/badge/python-3.11+-blue.svg)](https://www.python.org/downloads/)
[![PlatformIO](https://img.shields.io/badge/build-PlatformIO-orange.svg)](https://platformio.org/)

![Full Setup](images/full-setup-dark.jpg)

> This project was built on an [Indiedroid Nova](https://ameridroid.com/products/indiedroid-nova?ref=ioqothsk) but works identically on a **Raspberry Pi 5** — see [Raspberry Pi Setup Guide](docs/raspberry-pi-setup.md) for a quick start.

## What It Does

The CrowPanel acts as a compact heads-up display for an SDR scanning station:

- **LIVE/DEMO** status indicator
- **Active band** and real-time **frequency readout**
- **SNR bar** (color-coded) + **30-point scrolling SNR history chart**
- **ML signal classifier** with confidence percentage
- **Scan and anomaly counters**
- **System telemetry** — CPU, RAM, temperature (°F arc gauge), uptime
- Polls the SBC every 2 seconds over HTTP

The CrowView Note displays the full web dashboard with spectrum waterfall, anomaly timeline, and intelligence analysis — turning the whole setup into a portable cyberdeck laptop.

![CrowPanel Closeup](images/crowpanel-closeup.jpg)

## Compatibility

This project runs on **any Linux SBC** with USB and WiFi/Ethernet. Tested and compatible hardware:

| Platform | Status | Notes |
|----------|--------|-------|
| **Raspberry Pi 5** | ✅ Recommended | Best community support, widely available |
| **Raspberry Pi 4** | ✅ Compatible | Slightly slower, still works great |
| [Indiedroid Nova](https://ameridroid.com/products/indiedroid-nova?ref=ioqothsk) | ✅ Power User | RK3588S, 16GB RAM — what this demo was built on |
| **Jetson Nano** | ✅ Compatible | NVIDIA GPU bonus for ML workloads |
| **Orange Pi 5** | ✅ Compatible | RK3588S, good Nova alternative |
| **Any ARM64/x86 Linux** | ✅ Compatible | 4GB+ RAM, USB 2.0+, Python 3.11+ |

**You just need:** USB port (for SDR dongle) + network (for CrowPanel to reach the SBC).

## Hardware

| Component | Details |
|-----------|---------|
| **Compute** | **Raspberry Pi 5** (recommended) or [Indiedroid Nova](https://ameridroid.com/products/indiedroid-nova?ref=ioqothsk) (power user) |
| **SDR** | RTL-SDR Blog V4 (USB) |
| **Status Display** | [CrowPanel 7.0"](https://www.elecrow.com/crowpanel-7-0-esp32-hmi-display.html) — ESP32-S3, 800×480 RGB, capacitive touch |
| **Main Display** | [CrowView Note](https://www.elecrow.com/crowview-note-15-6-all-in-one-portable-monitor-phone-to-laptop-device.html?utm_source=Trevor&utm_medium=social&utm_campaign=ABCD&idd=5) — 15.6" 1080p portable monitor (USB-C DP Alt Mode) |
| **Antenna** | Telescoping whip (pictured) |

Full parts list with purchase links: [hardware/parts-list.md](hardware/parts-list.md)

## Quick Start

### Raspberry Pi 5 Users

See the dedicated **[Raspberry Pi Setup Guide](docs/raspberry-pi-setup.md)** for the fastest path.

### General Setup

1. **Flash the CrowPanel** — Connect via USB, build with PlatformIO:
   ```bash
   cd crowpanel-dashboard
   pio run --target upload
   ```

2. **Run the stats sender** on your Pi/Nova/SBC:
   ```bash
   pip install psutil
   python3 nova-stats-sender/stats_sender.py
   ```

3. **Connect CrowPanel to WiFi** — Edit the WiFi credentials in `main.cpp` and reflash.

4. The CrowPanel will start polling `http://<your-sbc-ip>:8088/stats` every 2 seconds.

See the full [Setup Guide](docs/setup-guide.md) for details.

## How It Works

```
[RTL-SDR V4] ──USB──▶ [Raspberry Pi 5 / Nova / Any SBC]
                                    │
                              stats_sender.py
                              (port 8088)
                                    │
                              WiFi HTTP GET
                                    │
                           [CrowPanel ESP32-S3]
                            800×480 LVGL UI
```

The **stats sender** (`stats_sender.py`) reads live scanner output and system metrics, then serves a simple JSON endpoint. The CrowPanel firmware polls this endpoint and updates the LVGL widgets in real time. It runs on any Linux SBC — the code is hardware-agnostic.

The CrowView Note runs Chromium in kiosk mode pointing at the full web dashboard, creating a portable laptop-style cyberdeck.

## Repo Structure

```
crowview-cyberdeck/
├── crowpanel-dashboard/     # ESP32-S3 firmware (PlatformIO + LVGL + LovyanGFX)
│   ├── src/
│   │   ├── main.cpp         # Full LVGL UI + HTTP polling
│   │   └── display_driver.h # LovyanGFX RGB LCD driver
│   └── platformio.ini
├── nova-stats-sender/       # Python bridge server (runs on any Linux SBC)
│   └── stats_sender.py      # Reads scanner data, serves JSON on port 8088
├── hardware/
│   └── parts-list.md
├── images/
├── docs/
│   ├── setup-guide.md
│   ├── raspberry-pi-setup.md  # Quick start for Pi 5 users
│   └── troubleshooting.md
└── README.md
```

## Related Projects

- **[rtl-ml](https://github.com/TrevTron/rtl-ml)** — AI-powered radio signal classification using RTL-SDR and machine learning. The ML classifier referenced in the CrowPanel UI connects to this project.

## Tech Stack

- **CrowPanel Firmware:** C++ / Arduino framework, LVGL 8.3, LovyanGFX, ArduinoJson
- **Stats Sender:** Python 3, psutil (runs on any Linux)
- **Build System:** PlatformIO (espressif32@6.5.0)
- **Display Driver:** Custom LovyanGFX class with PCA9557 I2C GPIO expander for touch reset

## Gallery

| | |
|---|---|
| ![Setup Dark](images/full-setup-dark.jpg) | ![Setup Lit](images/full-setup-lit.jpg) |
| ![CrowPanel](images/crowpanel-closeup.jpg) | ![Wide View](images/three-screen-wide.jpg) |

## Acknowledgments

- **[Elecrow](https://www.elecrow.com/)** — [CrowPanel 7.0" ESP32-S3 HMI display](https://www.elecrow.com/crowpanel-7-0-esp32-hmi-display.html) and [CrowView Note portable monitor](https://www.elecrow.com/crowview-note-15-6-all-in-one-portable-monitor-phone-to-laptop-device.html?utm_source=Trevor&utm_medium=social&utm_campaign=ABCD&idd=5)
- **[AmeriDroid](https://ameridroid.com/?ref=ioqothsk)** — Indiedroid Nova hardware partner

## License

MIT License — see [LICENSE](LICENSE) for details.
