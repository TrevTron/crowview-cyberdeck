# Parts List

## Core Components

| Component | Model | Approx. Price | Notes |
|-----------|-------|---------------|-------|
| Single-Board Computer | **Raspberry Pi 5** (8GB recommended) | ~$80 | Default choice. Any Linux SBC works — see compatibility below. |
| SBC (Power User Alt.) | [Indiedroid Nova](https://ameridroid.com/products/indiedroid-nova?ref=ioqothsk) | ~$100 | RK3588S, 16GB LPDDR4X. What this demo was built on. |
| SDR Dongle | [RTL-SDR Blog V4](https://www.rtl-sdr.com/rtl-sdr-blog-v4/) | ~$35 | USB, 500 kHz – 1.7 GHz |
| Status Display | [CrowPanel 7.0" ESP32-S3](https://www.elecrow.com/crowpanel-7-0-esp32-hmi-display.html) | ~$50 | ESP32-S3-WROOM-1-N4R8, 800×480 RGB LCD, GT911 capacitive touch |
| Main Display | CrowView Note 15.6" | ~$200 | 1920×1080 IPS, USB-C DisplayPort Alt Mode |
| Antenna | Telescoping whip antenna | ~$10 | SMA connector, for the RTL-SDR |

## Cables & Accessories

| Item | Notes |
|------|-------|
| USB-C cable (data) | SBC to CrowPanel for flashing firmware |
| USB-C cable (DP Alt Mode) | SBC to CrowView Note for display output |
| USB-A to USB-C adapter | If your SBC only has USB-A ports |
| MicroSD card | For Pi OS / Nova OS |
| Power supply | 5V/5A USB-C for Pi 5, 5V/3A for Nova, 5V USB for CrowPanel |
| Mini tripod (optional) | For mounting the CrowPanel at a viewable angle |

## Recommended for Raspberry Pi 5

| Item | Notes |
|------|-------|
| Raspberry Pi 5 active cooler | Recommended for sustained SDR workloads |
| Official Pi 5 27W USB-C PSU | Required for stable USB peripheral power |
| Elecrow CrowVision HDMI display | Alternative if you want a second Elecrow panel instead of the CrowView Note |

## Compatibility

The stats sender (`stats_sender.py`) and CrowPanel firmware are **hardware-agnostic** — they work on any Linux SBC:

- **Raspberry Pi 5** — Recommended. Best community support, widely available.
- **Raspberry Pi 4** — Slightly slower, still works great.
- **[Indiedroid Nova](https://ameridroid.com/products/indiedroid-nova?ref=ioqothsk)** — Power user choice. RK3588S with 16GB RAM. This is what the original demo was built on.
- **Jetson Nano** — NVIDIA GPU is a bonus for ML workloads.
- **Orange Pi 5** — RK3588S, good Nova alternative.
- **Any ARM64/x86 Linux** with 4GB+ RAM, USB 2.0+, Python 3.11+.

## Substitutions

- **Any Linux SBC** can replace the compute board. You just need USB for the SDR and WiFi/Ethernet for the CrowPanel to reach it. Raspberry Pi 5 is the easiest option.
- **Any RTL-SDR** compatible dongle works. The Blog V4 has the best filtering but the generic ones will scan too.
- **The CrowView Note is optional.** The CrowPanel works standalone as a compact status display. You can also view the web dashboard from any browser on your network.
