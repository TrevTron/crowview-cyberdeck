# Parts List

## Core Components

| Component | Model | Approx. Price | Notes |
|-----------|-------|---------------|-------|
| Single-Board Computer | [Indiedroid Nova](https://indiedroid.us/) | ~$100 | RK3588S, 16GB LPDDR4X. Any Linux SBC works — Pi 4/5, Orange Pi, etc. |
| SDR Dongle | [RTL-SDR Blog V4](https://www.rtl-sdr.com/rtl-sdr-blog-v4/) | ~$35 | USB, 500 kHz – 1.7 GHz |
| Status Display | [CrowPanel 7.0" ESP32-S3](https://www.elecrow.com/crowpanel-7-0-esp32-hmi-display.html) | ~$50 | ESP32-S3-WROOM-1-N4R8, 800×480 RGB LCD, GT911 capacitive touch |
| Main Display | CrowView Note 15.6" | ~$200 | 1920×1080 IPS, USB-C DisplayPort Alt Mode |
| Antenna | Telescoping whip antenna | ~$10 | SMA connector, for the RTL-SDR |

## Cables & Accessories

| Item | Notes |
|------|-------|
| USB-C cable (data) | Nova to CrowPanel for flashing firmware |
| USB-C cable (DP Alt Mode) | Nova to CrowView Note for display output |
| USB-A to USB-C adapter | If your SBC only has USB-A ports |
| MicroSD card | For Nova OS (if not using eMMC/NVMe) |
| Power supply | 5V/3A USB-C for Nova, 5V USB for CrowPanel |
| Mini tripod (optional) | For mounting the CrowPanel at a viewable angle |

## Substitutions

- **Any Linux SBC** can replace the Indiedroid Nova. You just need USB for the SDR and WiFi/Ethernet for the CrowPanel to reach it. Raspberry Pi 4/5 works fine.
- **Any RTL-SDR** compatible dongle works. The Blog V4 has the best filtering but the generic ones will scan too.
- **The CrowView Note is optional.** The CrowPanel works standalone as a compact status display. You can also view the web dashboard from any browser on your network.
