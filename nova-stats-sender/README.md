# Nova Stats Sender

A lightweight Python HTTP bridge that reads live RF scanner data and system metrics, then serves them as a JSON endpoint for the CrowPanel to poll.

## Usage

```bash
pip install psutil
python3 stats_sender.py
```

Serves on port 8088 at `GET /stats`.

## Configuration

Edit the `DATA_JSON` path at the top of `stats_sender.py` to point to your scanner's output file:

```python
DATA_JSON = "/mnt/rfdata/scans/data.json"
```

## JSON Output

```json
{
  "sdr_active": true,
  "band": "315-390 MHz UHF",
  "freq_mhz": 351.123,
  "snr": 12.7,
  "peak_power": 37.0,
  "noise_floor": 24.3,
  "scan_count": 8038,
  "anomaly_count": 1000,
  "classification": "ELF PULSE",
  "confidence": 41.9,
  "cpu": 10.5,
  "ram": 11.5,
  "temp": 44.4,
  "uptime": "0:02:20",
  "timestamp": "2026-04-08T20:18:32.944956"
}
```

## Running as a Service

See [docs/setup-guide.md](../docs/setup-guide.md) for systemd instructions.
