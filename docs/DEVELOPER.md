# Developer Guide

## Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VSCode extension)
- Python 3.11+
- Git
- A `secrets.h` file in `include/` (copy from `secrets.h.example`)

## Local Development

### Build firmware
```bash
pio run
```

### Build and upload via serial (USB)
```bash
pio run --target upload
```

### Build and upload via OTA (over WiFi)
```bash
# Windows (PowerShell)
$env:OTA_PASSWORD="smoker2026"; pio run --target upload --upload-port 10.10.63.242

# Windows (cmd)
python -c "import subprocess,os; env=os.environ.copy(); env['OTA_PASSWORD']='smoker2026'; subprocess.run(['pio','run','--target','upload','--upload-port','10.10.63.242'], env=env)"

# Linux/macOS
OTA_PASSWORD=smoker2026 pio run --target upload --upload-port 10.10.63.242
```

The device hostname `esp32-smoker.local` can be used instead of the IP if mDNS is working on your network.

### Monitor serial output
```bash
pio device monitor
```

## Web Interface Development

The web UI source files live in `data/www/`:
- `index.html` - Dashboard layout
- `style.css` - Styles
- `script.js` - Client-side logic

These files are **embedded in firmware** via PROGMEM (not served from a filesystem). After editing any web file, you must regenerate `include/web_content.h` before building.

### Regenerating web_content.h

The file wraps each source file in a C++ raw string literal:
```cpp
const char web_index_html[] PROGMEM = R"rawliteral(
...contents of index.html...
)rawliteral";
```

This is currently done manually (or by Claude Code). The structure is straightforward - just wrap each file's contents between `R"rawliteral(` and `)rawliteral";`.

### Fast OTA Check Mode

During active development, toggle "Fast OTA" in the web UI's Debug panel to check for updates every 60 seconds instead of every 6 hours. This is useful when testing the CI/CD pipeline.

## CI/CD Pipeline

### How it works

The GitHub Actions workflow (`.github/workflows/build.yml`) handles two scenarios:

1. **Push to `main` or PR** - Builds firmware to verify compilation
2. **Version tag (`v*`)** - Builds firmware + creates a GitHub Release with `firmware.bin` and `version.txt`

The ESP32 periodically checks the latest GitHub Release for a newer `version.txt`. If found (and the smoker is idle), it downloads and flashes `firmware.bin` automatically.

### Required GitHub Secrets

Configure these in **Settings > Secrets and variables > Actions**:

| Secret | Description |
|--------|-------------|
| `WIFI_SSID` | WiFi network name |
| `WIFI_PASS` | WiFi password |
| `WIFI_AP_PASS` | Fallback AP mode password |
| `MQTT_BROKER_HOST` | MQTT broker IP/hostname |
| `MQTT_USERNAME` | MQTT username |
| `MQTT_PASSWORD` | MQTT password |
| `OTA_PASSWORD` | ArduinoOTA password |
| `SYSLOG_SERVER` | Syslog server IP |

### Releasing a New Version

1. Make your code changes
2. Bump `FIRMWARE_VERSION` in `include/config.h`:
   ```cpp
   #define FIRMWARE_VERSION  "1.2.0"
   ```
3. Commit and push to `main`
4. Tag and push the tag:
   ```bash
   git tag v1.2.0
   git push origin v1.2.0
   ```
5. GitHub Actions builds and creates the release automatically
6. The ESP32 will detect and install the update within 6 hours (or 60 seconds in Fast OTA mode)

### Verifying a Release

- Check the [Actions tab](https://github.com/briangunderson/ESP32Smoker/actions) for build status
- Check [Releases](https://github.com/briangunderson/ESP32Smoker/releases) for the published `firmware.bin` and `version.txt`
- On the ESP32 web UI, open Debug panel and click "Check for Updates" to trigger an immediate check

## Project Structure

```
ESP32Smoker/
  include/
    config.h          - All configuration constants
    secrets.h          - Credentials (gitignored)
    secrets.h.example  - Template for secrets.h
    web_content.h      - Generated PROGMEM web content
    http_ota.h         - HTTP OTA module header
    ...
  src/
    main.cpp           - Entry point, setup/loop
    temperature_control.cpp - State machine & PID
    web_server.cpp     - REST API + web serving
    http_ota.cpp       - Pull-based OTA from GitHub Releases
    ...
  data/www/
    index.html         - Web UI source (edit here)
    style.css          - Styles source (edit here)
    script.js          - JS source (edit here)
  .github/workflows/
    build.yml          - CI/CD pipeline
  docs/
    API.md             - REST API reference
    ARCHITECTURE.md    - System design
    DEVELOPER.md       - This file
```

## Troubleshooting

### OTA upload fails with "Authentication Failed"
The `OTA_PASSWORD` environment variable must be set. On Windows CMD, `set VAR=value && command` doesn't reliably pass env vars to PlatformIO. Use the Python subprocess method shown above, or PowerShell.

### Build fails with "LOG_INFO not declared"
Make sure `#include "logger.h"` is present in any file using `LOG_INFO`, `LOG_ERR`, etc.

### Web UI shows old content after editing
You forgot to regenerate `include/web_content.h`. The source files in `data/www/` are only used as source for the PROGMEM embedding - the ESP32 serves from `web_content.h`.
