# Raspberry Pi 5 Setup Guide

Quick start guide for running the CrowPanel RF dashboard on a **Raspberry Pi 5**. This is the recommended setup for most users.

> This project was originally built on an [Indiedroid Nova](https://ameridroid.com/products/indiedroid-nova?ref=ioqothsk) (RK3588S) but the stats sender and CrowPanel firmware are hardware-agnostic. Everything below works identically on Pi 5.

## What You Need

| Component | Notes |
|-----------|-------|
| Raspberry Pi 5 (4GB or 8GB) | 8GB recommended for running scanner + dashboard |
| RTL-SDR Blog V4 | USB SDR dongle — any RTL-SDR works |
| [CrowPanel 7.0" ESP32-S3](https://www.elecrow.com/crowpanel-7-0-esp32-hmi-display.html) | The HMI status display |
| MicroSD card (32GB+) | For Raspberry Pi OS |
| Official Pi 5 27W USB-C PSU | Needed for stable USB peripheral power |
| Antenna | Telescoping whip or any SMA antenna |

**Optional:**
- [CrowView Note 15.6"](https://www.elecrow.com/crowview-note-15-6-all-in-one-portable-monitor-phone-to-laptop-device.html?utm_source=Trevor&utm_medium=social&utm_campaign=ABCD&idd=5) portable monitor (for full dashboard view)
- Active cooler for Pi 5 (recommended for sustained SDR scanning)

## Step 1: Set Up Raspberry Pi OS

1. Flash **Raspberry Pi OS (64-bit, Bookworm)** using [Raspberry Pi Imager](https://www.raspberrypi.com/software/).
2. Boot, connect to WiFi, and run updates:
   ```bash
   sudo apt update && sudo apt upgrade -y
   ```
3. Note your Pi's IP address:
   ```bash
   hostname -I
   ```

## Step 2: Install Dependencies

```bash
sudo apt install -y python3-pip python3-venv git
python3 -m venv ~/cyberdeck-env
source ~/cyberdeck-env/bin/activate
pip install psutil
```

## Step 3: Clone the Repo

```bash
git clone https://github.com/TrevTron/crowview-cyberdeck.git
cd crowview-cyberdeck
```

## Step 4: Run the Stats Sender

The stats sender bridges your scanner data to the CrowPanel over HTTP:

```bash
source ~/cyberdeck-env/bin/activate
python3 nova-stats-sender/stats_sender.py
```

It will serve JSON on port 8088. Test it from another device:
```bash
curl http://<your-pi-ip>:8088/stats
```

You should see JSON with CPU, RAM, temperature, and scanner data (if a scanner is running).

### Run as a Service (Optional)

To keep it running on boot:

```bash
sudo tee /etc/systemd/system/rf-bridge.service << 'EOF'
[Unit]
Description=RF Bridge — CrowPanel Stats Server
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/crowview-cyberdeck
ExecStart=/home/pi/cyberdeck-env/bin/python3 nova-stats-sender/stats_sender.py
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now rf-bridge
```

## Step 5: Flash the CrowPanel

1. Connect the CrowPanel to your PC (not the Pi) via USB-C.
2. Install [PlatformIO](https://platformio.org/install/cli) if you haven't.
3. Edit `crowpanel-dashboard/src/main.cpp` — set your WiFi credentials and Pi's IP:
   ```cpp
   #define WIFI_SSID     "YourNetwork"
   #define WIFI_PASS     "YourPassword"
   #define NOVA_STATS_IP "192.168.x.x"  // Your Pi's IP
   ```
4. Build and flash:
   ```bash
   cd crowpanel-dashboard
   pio run --target upload
   ```

The CrowPanel will connect to WiFi and start polling your Pi's stats endpoint every 2 seconds.

## Step 6: CrowView Note (Optional)

If you have a CrowView Note or any USB-C DisplayPort monitor:

1. Connect via USB-C to the Pi 5's USB-C port (the one not used for power).
2. Open Chromium in kiosk mode:
   ```bash
   chromium-browser --kiosk http://localhost:8080
   ```

This gives you the full web dashboard alongside the CrowPanel HMI display.

## Troubleshooting

- **CrowPanel shows "NO DATA"** — Verify the stats sender is running and the Pi's IP matches what's in `main.cpp`.
- **Temperature reads 0** — Normal on some Pi OS versions. The sensor name varies; the code tries `cpu_thermal`, `soc_thermal`, and sysfs fallback automatically.
- **USB power warnings** — Use the official 27W Pi 5 PSU. The RTL-SDR draws significant current.
- **WiFi drops** — The CrowPanel will auto-reconnect. If the Pi drops, check `journalctl -u rf-bridge`.

See [troubleshooting.md](troubleshooting.md) for more common issues.

## Next Steps

- Check out **[rtl-ml](https://github.com/TrevTron/rtl-ml)** to add AI signal classification to your scanner.
- See the full [setup guide](setup-guide.md) for advanced configuration (systemd services, Chromium autostart, scanner integration).
