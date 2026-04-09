#!/usr/bin/env python3
"""
stats_sender.py — Bridge server for CrowPanel RF dashboard.

Reads live scanner data and system metrics, serves JSON on port 8088
in the format the CrowPanel ESP32-S3 firmware expects.

COMPATIBILITY: Runs on any Linux SBC — Raspberry Pi 5/4, Indiedroid Nova,
Jetson Nano, Orange Pi, or any ARM64/x86 machine with Python 3.11+ and psutil.
Originally built on an Indiedroid Nova (RK3588S) but hardware-agnostic.

Usage:
    pip install psutil
    python3 stats_sender.py

The CrowPanel polls http://<your-sbc-ip>:8088/stats every 2 seconds.
"""

import json
import os
import re
import time
import psutil
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime

DATA_JSON = "/mnt/rfdata/scans/data.json"
CACHE_TTL = 2  # seconds

_cache = {"data": None, "mtime": 0, "ts": 0}
_anomaly_idx = 0  # rotates through recent anomalies each request


def read_scanner_data():
    """Read and cache data.json from scanner."""
    now = time.time()
    if _cache["data"] and (now - _cache["ts"]) < CACHE_TTL:
        return _cache["data"]
    try:
        mtime = os.path.getmtime(DATA_JSON)
        if mtime != _cache["mtime"] or _cache["data"] is None:
            with open(DATA_JSON) as f:
                _cache["data"] = json.load(f)
            _cache["mtime"] = mtime
        _cache["ts"] = now
        return _cache["data"]
    except Exception as e:
        print(f"Error reading data.json: {e}")
        return _cache["data"]  # return stale if available


def get_uptime():
    boot = psutil.boot_time()
    elapsed = time.time() - boot
    h, rem = divmod(int(elapsed), 3600)
    m, s = divmod(rem, 60)
    return f"{h}:{m:02d}:{s:02d}"


def get_cpu_temp():
    """Read CPU temperature. Works across SBCs — tries common sensor names
    (soc_thermal for RK3588S/Nova, cpu_thermal for Pi, coretemp for x86)
    and falls back to sysfs thermal_zone0."""
    try:
        temps = psutil.sensors_temperatures()
        for name in ("soc_thermal", "cpu_thermal", "coretemp"):
            if name in temps and temps[name]:
                return temps[name][0].current
        # Fallback: read sysfs
        with open("/sys/class/thermal/thermal_zone0/temp") as f:
            return int(f.read().strip()) / 1000.0
    except:
        return 0.0


def build_stats():
    """Build the stats JSON matching CrowPanel firmware expectations."""
    d = read_scanner_data()

    if d is None:
        return {
            "sdr_active": False,
            "band": "NO DATA",
            "freq_mhz": 0.0,
            "snr": 0.0,
            "peak_power": 0.0,
            "noise_floor": 0.0,
            "scan_count": 0,
            "anomaly_count": 0,
            "classification": "---",
            "confidence": 0.0,
            "cpu": psutil.cpu_percent(),
            "ram": psutil.virtual_memory().percent,
            "temp": get_cpu_temp(),
            "uptime": get_uptime(),
            "timestamp": datetime.now().isoformat(),
        }

    stats = d.get("stats", {})
    anomalies = d.get("anomalies", [])
    type_counts = d.get("type_counts", {})

    # Rotate through recent anomalies so each poll gets a different data point
    global _anomaly_idx
    if anomalies:
        _anomaly_idx = (_anomaly_idx + 1) % min(len(anomalies), 50)
        latest = anomalies[_anomaly_idx]
    else:
        latest = {}
    freq = latest.get("freq_mhz", d.get("center_mhz", 0.0))
    power = latest.get("power_db", 0.0)
    noise = latest.get("noise_db", 0.0)
    snr = power - noise if power and noise else 0.0

    # Classification: most common anomaly type
    if type_counts:
        top_type = max(type_counts, key=type_counts.get)
        top_count = type_counts[top_type]
        total = sum(type_counts.values())
        confidence = (top_count / total * 100) if total > 0 else 0.0
    else:
        top_type = "---"
        confidence = 0.0

    # Band label — short ASCII-safe string for CrowPanel
    band_label = d.get("band_label", d.get("band", "---"))
    # Replace en-dash/em-dash with plain hyphen, strip to fit display
    band_short = band_label.replace("\u2013", "-").replace("\u2014", "-")
    # Extract just the MHz range portion: "315-390 MHz"
    m = re.search(r'(\d+)\s*-\s*(\d+)\s*MHz', band_short)
    if m:
        band_short = f"{m.group(1)}-{m.group(2)} MHz UHF"
    elif len(band_short) > 20:
        band_short = band_short[:20]

    return {
        "sdr_active": True,
        "band": band_short,
        "freq_mhz": round(freq, 3),
        "snr": round(snr, 1),
        "peak_power": round(power, 1),
        "noise_floor": round(noise, 1),
        "scan_count": stats.get("scan_count", 0),
        "anomaly_count": stats.get("total_anomalies", 0),
        "classification": top_type.replace("_", " "),
        "confidence": round(confidence, 1),
        "cpu": round(psutil.cpu_percent(), 1),
        "ram": round(psutil.virtual_memory().percent, 1),
        "temp": round(get_cpu_temp(), 1),
        "uptime": get_uptime(),
        "timestamp": datetime.now().isoformat(),
    }


class BridgeHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path in ("/stats", "/"):
            payload = json.dumps(build_stats()).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, fmt, *args):
        pass


def main():
    port = 8088
    server = HTTPServer(("0.0.0.0", port), BridgeHandler)
    print(f"RF Bridge serving on port {port} — live scanner data → CrowPanel")
    print(f"CrowPanel should poll http://<this-ip>:{port}/stats")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
        server.server_close()


if __name__ == "__main__":
    main()
