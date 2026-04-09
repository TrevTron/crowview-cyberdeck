# Setup Guide

## Prerequisites

- An SBC running Linux (Indiedroid Nova, Raspberry Pi, etc.)
- Python 3.9+ with `psutil` installed (`pip install psutil`)
- PlatformIO CLI or PlatformIO IDE ([install guide](https://platformio.org/install))
- CrowPanel 7.0" ESP32-S3 connected via USB
- An RTL-SDR dongle (or any scanner that writes JSON data)

## 1. Flash the CrowPanel Firmware

### Connect the CrowPanel

Plug the CrowPanel into your SBC (or any computer) via USB-C. It shows up as a CH340 serial device:

- **Linux:** `/dev/ttyUSB0`
- **macOS:** `/dev/cu.usbserial-*`
- **Windows:** `COM3` or similar (check Device Manager)

### Configure WiFi

Edit `crowpanel-dashboard/src/main.cpp` and update the WiFi credentials near the top of the file:

```cpp
#define WIFI_SSID     "YourNetworkName"
#define WIFI_PASS     "YourPassword"
```

### Set the Nova/SBC IP

In the same file, update the IP address the CrowPanel polls:

```cpp
#define NOVA_HOST     "192.168.x.x"    // Your SBC's local IP
#define NOVA_STATS_PORT  8088
```

### Build and Flash

```bash
cd crowpanel-dashboard
pio run --target upload --upload-port /dev/ttyUSB0
```

The CrowPanel will reboot and start trying to connect to WiFi.

## 2. Set Up the Stats Sender

The stats sender is a lightweight Python HTTP server that reads your scanner's JSON output and serves it in the format the CrowPanel expects.

### Install Dependencies

```bash
pip install psutil
```

### Configure the Data Path

Edit `nova-stats-sender/stats_sender.py` and update the path to your scanner's output file:

```python
DATA_JSON = "/path/to/your/scanner/data.json"
```

The data.json should contain at minimum:
- `band_label` — string describing the active band
- `center_mhz` — center frequency
- `anomalies` — array of objects with `freq_mhz`, `power_db`, `noise_db`, `type`
- `stats.scan_count` and `stats.total_anomalies`

### Run It

```bash
python3 nova-stats-sender/stats_sender.py
```

It will serve on port 8088. Test it:

```bash
curl http://localhost:8088/stats
```

You should see JSON with `sdr_active`, `band`, `freq_mhz`, `snr`, etc.

### Run as a System Service (Optional)

Create `/etc/systemd/system/rf-bridge.service`:

```ini
[Unit]
Description=RF Stats Bridge for CrowPanel
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /path/to/stats_sender.py
Restart=always
RestartSec=3
User=your-username

[Install]
WantedBy=multi-user.target
```

Then:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now rf-bridge
```

## 3. Set Up the CrowView Note (Optional)

If you're using a secondary display for the full web dashboard:

### Install Chromium

```bash
sudo apt install chromium
```

### Launch in Kiosk Mode

```bash
chromium --kiosk --no-first-run --disable-translate --disable-infobars \
  --disable-session-crashed-bubble --noerrdialogs \
  --ozone-platform=wayland http://YOUR-SBC-IP:8080
```

Replace `--ozone-platform=wayland` with `--ozone-platform=x11` if you're running X11 instead of Wayland.

## 4. Verify Everything

1. **CrowPanel** should show "LIVE" in green with your band, frequency, and SNR updating every 2 seconds
2. **Stats sender** should respond to `curl http://localhost:8088/stats` with fresh JSON
3. **CrowView Note** (if used) should show the full web dashboard
