# Troubleshooting

## CrowPanel Issues

### Display shows "DEMO" instead of "LIVE"

The CrowPanel can't reach the stats sender. Check:

1. **Is the stats sender running?**
   ```bash
   curl http://localhost:8088/stats
   ```
   If connection refused, start it: `python3 stats_sender.py`

2. **Is the CrowPanel on WiFi?**
   The CrowPanel prints WiFi status to serial. Connect via USB and open a serial monitor at 115200 baud.

3. **Is the IP correct?**
   The CrowPanel firmware has `NOVA_HOST` hardcoded. Make sure it matches your SBC's actual local IP.

4. **Firewall?**
   Make sure port 8088 is open:
   ```bash
   sudo ufw allow 8088/tcp
   ```

### Display is garbled or white

This is usually a display driver issue. The CrowPanel 7" V3.0 uses an RGB LCD interface that shares buses with PSRAM on the ESP32-S3.

- Make sure you're using the LovyanGFX display driver included in `src/display_driver.h`
- Don't change the pixel clock above 12MHz — it causes corruption
- If you modified the LGFX class, double-check the GPIO pin mappings match your CrowPanel version

### SNR chart is a flat line

The stats sender might be returning the same anomaly data on every request. Check that:

- Your scanner is actively writing new data to `data.json`
- The stats sender's `_anomaly_idx` rotation is working (it should cycle through recent anomalies)
- Test with: `for i in range(5): curl http://localhost:8088/stats` — the `snr` value should vary

### Non-ASCII characters show as squares

The CrowPanel firmware only includes basic ASCII glyphs. The stats sender should convert special characters (en-dash, em-dash, etc.) to ASCII equivalents. If you see squares, check the `band` field in the stats JSON.

### Touch screen not responding

The CrowPanel uses a GT911 touch controller that requires a specific reset sequence via the PCA9557 I2C GPIO expander. This is handled in `display_driver.h` → `crowpanel_gpio_init()`. If touch isn't working:

- Check I2C pull-ups
- Make sure the PCA9557 reset sequence runs before LVGL init

## Stats Sender Issues

### "No such file" error on startup

The `DATA_JSON` path in `stats_sender.py` doesn't exist. Update it to point to your scanner's actual output file.

### JSON shows stale data

The stats sender caches data.json for 2 seconds (`CACHE_TTL`). If your scanner writes infrequently, you may see repeated values. This is normal — the CrowPanel will update when new data arrives.

### psutil can't read CPU temperature

Some SBCs don't expose temperature through psutil's `sensors_temperatures()`. The stats sender falls back to reading `/sys/class/thermal/thermal_zone0/temp`. If neither works, temperature will show as 0°C.

Check available thermal zones:
```bash
ls /sys/class/thermal/thermal_zone*/temp
cat /sys/class/thermal/thermal_zone0/temp
```

## CrowView Note Issues

### Chromium won't launch via SSH

You need to set the Wayland/X11 session variables:

```bash
export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/bus
export XDG_RUNTIME_DIR=/run/user/$(id -u)
export WAYLAND_DISPLAY=wayland-0    # or DISPLAY=:0 for X11
```

### GNOME Keyring password prompt on first launch

Press Enter twice (leave blank) to set an empty keyring password. This only happens once.

### Dashboard shows "connection error"

- Check that the web dashboard server is running on port 8080
- Make sure you're loading the page via the SBC's IP, not `localhost` (avoids CORS issues)
- Hard refresh: Ctrl+Shift+R

### Browser freezes

Firefox-ESR is known to freeze on ARM SBCs. Use Chromium instead:
```bash
sudo apt install chromium
```
Launch with `--disable-session-crashed-bubble --noerrdialogs` to prevent recovery popups.
