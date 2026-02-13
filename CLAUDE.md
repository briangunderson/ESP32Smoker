# ESP32 Wood Pellet Smoker Controller - Context for Claude

## Project Overview
This is a complete, production-ready wood pellet smoker controller built on ESP32 with Arduino framework. It features real-time temperature control, web interface, and Home Assistant integration via MQTT.

**Status**: Hardware working (PT1000 + 4300Ω ref on second MAX31865 board)
**Version**: 1.2.0 (Deferred OTA check + graph range buttons + first GitHub release)
**Last Build**: February 12, 2026
**Product Name**: GunderGrill

## Quick Facts
- **Platform**: ESP32 (PlatformIO + Arduino Framework)
- **Language**: C++ for firmware, HTML/CSS/JS for web interface
- **Temperature Sensor**: MAX31865 RTD (PT1000 probe + 4300Ω ref via SPI)
- **Display**: TM1638 (dual 7-segment displays, 8 buttons, 8 LEDs)
- **Input**: M5Stack Unit Encoder (U135) rotary encoder via I2C (setpoint adjust + start/stop)
- **Control**: 3 relays (auger, fan, igniter) with safety interlocks, active-low drive
- **Connectivity**: WiFi (STA/AP modes) + Web API + MQTT
- **Lines of Code**: ~4000 total (~1200 firmware, ~950 web UI, ~1500 docs, ~3700 logging infrastructure)

## CRITICAL: User Preferences

**ALWAYS AUTO-UPLOAD VIA OTA**: After any successful firmware build, automatically upload to the ESP32 via OTA without asking for permission. Use:
```bash
pio run --target upload --upload-port esp32-smoker.local
```

Only skip OTA upload if there's a technical reason preventing it:
- Device is offline or unreachable
- Network connectivity issues
- OTA authentication failures

**DO NOT** ask the user if they want to upload - just proceed automatically after builds complete successfully.

## CRITICAL: Git & CI/CD Etiquette

**ALWAYS use feature branches** for any non-trivial changes. Multiple Claude instances may be working on this project simultaneously.

### Branch Workflow
1. **Create a branch** before making changes: `git checkout -b <type>/<short-description>` (e.g., `fix/chart-range`, `feat/new-sensor`)
2. **Commit to the branch**, not directly to `main`
3. **Merge to main** only after changes are verified working (build succeeds, OTA upload works)
4. **Push to remote** after merging: `git push origin main`
5. **Clean up** feature branches after merge: `git branch -d <branch-name>`

### Avoiding Conflicts
- **Check `git status` and `git log`** before starting work to see if another instance has made recent changes
- **Pull before branching**: `git pull origin main` to get latest changes
- **Never force-push** to main
- **Keep `web_content.h` in sync**: After editing any file in `data/www/`, always regenerate `include/web_content.h` to match. This file embeds the web UI into firmware via PROGMEM.
- **OTA_PASSWORD**: Pass `OTA_PASSWORD=YOUR-OTA-PASSWORD` as env var for OTA uploads (not stored in platformio.ini for security)

### Commit Standards
- Write meaningful commit messages explaining *why*, not just *what*
- Include `Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>` in all commits
- Don't bundle unrelated changes in a single commit

## Critical Files

### Configuration
- `include/config.h` - **ALL configuration in one place** (pins, temps, timings, non-secret defaults)
- `include/secrets.h` - **Credentials** (WiFi, MQTT, OTA, syslog IPs) — **GITIGNORED, never commit**
- `include/secrets.h.example` - Template for secrets.h (committed, placeholder values)

### Core Firmware (src/)
- `main.cpp` - Application entry point, initialization, main loop
- `temperature_control.cpp` - State machine & PID control (THE BRAIN)
- `max31865.cpp` - RTD sensor driver (SPI communication)
- `relay_control.cpp` - GPIO relay control with safety interlocks
- `web_server.cpp` - Async HTTP server + REST API
- `mqtt_client.cpp` - Home Assistant integration
- `tm1638_display.cpp` - Physical display and button interface
- `encoder.cpp` - M5Stack Unit Encoder I2C driver (rotary + button + RGB LED)

### Web Interface (data/www/)
- `index.html` - Responsive dashboard
- `style.css` - Modern UI styling
- `script.js` - Real-time updates, API calls

### Documentation
- `README.md` - Main documentation (features, usage, setup)
- `BUILD_SUMMARY.md` - What was built, statistics, next steps
- `QUICK_REFERENCE.md` - Cheat sheet for common tasks
- `docs/ARCHITECTURE.md` - System design, control algorithms
- `docs/API.md` - REST API reference
- `docs/DEVELOPER.md` - Build, release workflow, CI/CD pipeline
- `hardware/WIRING_DIAGRAM.md` - Complete wiring guide

### Logging Infrastructure (logging/)
- `logging/README.md` - Complete logging infrastructure documentation
- `logging/QUICK_REFERENCE.md` - Quick reference for logging
- `logging/elasticsearch/` - Elasticsearch configurations (index template, ingest pipeline, watchers)
- `logging/kibana/` - Kibana configurations (visualizations, dashboard)
- `logging/cribl/` - Cribl Stream pipeline configuration
- `logging/scripts/` - Setup and test scripts

## Architecture Highlights

### Temperature Control State Machine
```
IDLE → STARTUP → RUNNING → COOLDOWN → SHUTDOWN → IDLE
              ↓
            ERROR (on fault)
```

**STARTUP sequence**:
1. Pre-heat igniter (60s)
2. Start fan (5s delay)
3. Enable auger
4. Wait for temp to reach 115°F (absolute threshold)
5. Igniter automatically turns off at 100°F

**RUNNING mode**:
- PID control using Proportional Band method (from PiSmoker)
- Time-proportioning auger control with 20-second cycles
- Minimum 15% auger duty cycle to keep fire alive
- Fan runs continuously
- Igniter off

### Safety Features
1. **Auger Interlock**: Auger cannot run without fan (fire safety)
2. **Temperature Limits**: Emergency stop if T < 50°F or T > 500°F
3. **Sensor Monitoring**: 3 consecutive read errors → emergency stop
4. **Startup Timeout**: 180-second maximum before error state

### GPIO Pin Assignments (config.h)
```
SPI (MAX31865):
  CLK  = GPIO 36 (SCK)
  MOSI = GPIO 35 (MO)
  MISO = GPIO 37 (MI)
  CS   = GPIO 5  (D5)

Relays (active-low):
  Auger   = GPIO 12 (D12)
  Fan     = GPIO 13 (D13)
  Igniter = GPIO 10 (D10)

TM1638 Display:
  STB  = GPIO 6  (D6 - Strobe)
  CLK  = GPIO 9  (D9 - Clock)
  DIO  = GPIO 14 (A4 - Data I/O)

I2C (STEMMA QT):
  SDA  = GPIO 3
  SCL  = GPIO 4
  Encoder = 0x40 (M5Stack Unit Encoder U135)

Status:
  LED = GPIO 11 (D11)
```

## TM1638 Display & Physical Controls

### Display Features
The TM1638 module provides a physical interface with:
- **Dual 7-segment displays**: Show current temperature (left) and setpoint (right)
- **8 Status LEDs**: Visual indicators for system status
- **8 Push buttons**: Physical control without web interface

### LED Indicators (Left to Right)
1. **Auger** - Pellet auger motor is running
2. **Fan** - Combustion fan is running
3. **Igniter** - Hot rod igniter is active
4. **WiFi** - Connected to WiFi network
5. **MQTT** - Connected to MQTT broker
6. **Error** - System error or fault condition
7. **Running** - Smoking in progress (RUNNING state)
8. **Heartbeat** - Blinks every second (system alive indicator)

### Button Controls (Left to Right)
1. **Start** - Begin smoking sequence (starts at current setpoint)
2. **Stop** - Stop feeding pellets, initiate cooldown
3. **Temp Up** - Increase setpoint by 5°F (max 350°F)
4. **Temp Down** - Decrease setpoint by 5°F (min 150°F)
5. **Mode** - Reserved for future use (cycles display modes)
6-8. **Reserved** - Available for future features

### Button Behavior
- Buttons have 300ms debounce to prevent accidental double-presses
- Temperature changes are immediate and reflected on display
- Buttons work in parallel with web interface (both control same system)
- Button presses are logged to serial output for debugging

## Key Configuration Parameters

### Sensor Configuration (config.h)
- `MAX31865_RTD_RESISTANCE_AT_0`: 1000.0 (PT1000 = 1000Ω at 0°C)
- `MAX31865_REFERENCE_RESISTANCE`: 4300.0 (4.3kΩ reference resistor for PT1000)
- `MAX31865_WIRE_MODE`: 3 (3-wire RTD, most common configuration)

### Temperature Control (config.h)
- `TEMP_MIN_SETPOINT`: 150°F (minimum allowed)
- `TEMP_MAX_SETPOINT`: 350°F (maximum allowed)
- `TEMP_CONTROL_INTERVAL`: 2000ms (PID update rate)

### PID Parameters (config.h)
Uses Proportional Band method (from PiSmoker) - proven stable control:
- `PID_PROPORTIONAL_BAND`: 60.0°F (temperature range for proportional response)
- `PID_INTEGRAL_TIME`: 180.0s (time constant for integral term)
- `PID_DERIVATIVE_TIME`: 45.0s (time constant for derivative term)
- `AUGER_CYCLE_TIME`: 20000ms (20-second window for time-proportioning control)
- `PID_OUTPUT_MIN`: 0.15 (15% minimum duty cycle - keeps fire alive)
- `PID_OUTPUT_MAX`: 1.0 (100% maximum duty cycle)

Gains are auto-calculated: Kp = -1/PB = -0.0167, Ki = Kp/Ti = -0.0000926, Kd = Kp*Td = -0.75

### Timing (config.h)
- `IGNITER_PREHEAT_TIME`: 60000ms (1 minute)
- `FAN_STARTUP_DELAY`: 5000ms (5 seconds)
- `STARTUP_TIMEOUT`: 180000ms (3 minutes)

### Network (config.h)
- WiFi STA mode for configured network
- AP fallback mode: SSID "ESP32Smoker", password in config
- Web interface: http://device_ip or http://192.168.4.1 (AP mode)
- MQTT broker: Configure host/port/credentials in config.h
  - Authentication enabled with username/password

### Logging (config.h)
- Serial debug logging at 115200 baud
- Syslog remote logging (RFC 5424 over UDP port 514)
  - Configure server IP in `SYSLOG_SERVER`
  - Enable/disable with `ENABLE_SYSLOG`
  - Logs initialization, status updates, and errors

## REST API Endpoints

### Normal Operation
- `GET /api/status` - Current temp, setpoint, state, relay status
- `POST /api/start` - Start smoking (optional: `{"temp": 250}`)
- `POST /api/stop` - Stop feeding pellets, initiate cooldown
- `POST /api/shutdown` - Full system shutdown
- `POST /api/setpoint` - Update target temp: `{"temp": 225}`

### Debug/Testing Endpoints
- `GET /api/debug/status` - Get debug mode status: `{"debugMode": true/false}`
- `POST /api/debug/mode` - Enable/disable debug mode: FormData `enabled=true/false`
- `POST /api/debug/relay` - Manual relay control: FormData `relay=auger/fan/igniter`, `state=true/false`
- `POST /api/debug/temp` - Set temperature override: FormData `temp=70`
- `DELETE /api/debug/temp` - Clear temperature override, resume sensor reading
- `GET /api/debug/sensor` - Raw MAX31865 diagnostics: ADC value, resistance, fault status, all registers
- `POST /api/debug/reset` - Reset error state back to idle

## MQTT Topics

**Authentication**: Broker connection requires username and password (configured in config.h)

### Published (sensors)
- `home/smoker/sensor/temperature` - Current temp
- `home/smoker/sensor/setpoint` - Target temp
- `home/smoker/sensor/state` - System state
- `home/smoker/sensor/auger` - "on" or "off"
- `home/smoker/sensor/fan` - "on" or "off"
- `home/smoker/sensor/igniter` - "on" or "off"

### Subscribed (commands)
- `home/smoker/command/start` - Payload: temperature (float)
- `home/smoker/command/stop` - Any payload
- `home/smoker/command/setpoint` - Payload: temperature (float)

## PID Tuning

The controller uses the **Proportional Band method** from PiSmoker, which provides stable temperature control with minimal oscillation. The default values work well for most pellet smokers.

### Proportional Band Method Explained

Unlike standard PID where gains are set directly, the Proportional Band method uses three intuitive parameters:

1. **Proportional Band (PB)**: Temperature range over which output varies from 0% to 100%
   - At (setpoint - PB/2): Output = 100% (max heat)
   - At setpoint: Output = 50% (balanced)
   - At (setpoint + PB/2): Output = 0% (no heat)

2. **Integral Time (Ti)**: How long it takes integral term to match proportional term
   - Larger = slower to eliminate steady-state error, more stable
   - Smaller = faster correction, more aggressive

3. **Derivative Time (Td)**: How strongly derivative term resists temperature changes
   - Larger = stronger damping of oscillations
   - Smaller = quicker response but more overshoot

### Default Values (config.h)
```cpp
#define PID_PROPORTIONAL_BAND    60.0   // 60°F band (proven stable)
#define PID_INTEGRAL_TIME        180.0  // 3 minutes (slow, stable)
#define PID_DERIVATIVE_TIME      45.0   // 45 seconds (good damping)
```

These translate to calculated gains:
- Kp = -0.0167 (negative for reverse-acting control)
- Ki = -0.0000926 (very small, slow integration)
- Kd = -0.75 (moderate derivative action)

### Key Features of This Implementation

1. **0.5 Centering**: Proportional term is offset by 0.5, naturally balancing around setpoint
2. **Negative Gains**: Reverse-acting control (increase temp → decrease output)
3. **Derivative on Measurement**: Prevents "derivative kick" when setpoint changes
4. **Conservative Anti-Windup**: Limits integral contribution to 50% of output range
5. **Minimum Maintenance**: 15% minimum auger duty cycle keeps fire alive

### Tuning Tips

**Most users should NOT need to tune** - defaults are battle-tested from PiSmoker.

If you do need to adjust:

1. **Start with Proportional Band**:
   - Too much oscillation? **Increase PB** (e.g., 60 → 80)
   - Too sluggish? **Decrease PB** (e.g., 60 → 50)

2. **Adjust Integral Time if needed**:
   - Steady offset from setpoint? **Decrease Ti** (e.g., 180 → 150)
   - Overshoots then settles? **Increase Ti** (e.g., 180 → 200)

3. **Fine-tune Derivative Time**:
   - Oscillates even with large PB? **Increase Td** (e.g., 45 → 60)
   - Too slow to recover from disturbances? **Decrease Td** (e.g., 45 → 35)

### Monitoring PID Performance

Watch serial output for PID debugging (every 5 seconds during RUNNING):
```
[PID] Temp:220.5 Set:225.0 Err:-4.5 P:0.575 I:0.023 D:-0.012 Out:0.58
```

- **Temp**: Current temperature
- **Set**: Setpoint (target)
- **Err**: Current - Setpoint (reversed from standard)
- **P**: Proportional contribution (should be ~0.5 at setpoint)
- **I**: Integral contribution (accumulates over time)
- **D**: Derivative contribution (resists temperature changes)
- **Out**: Final output (0.15 to 1.0 = 15% to 100% auger duty cycle)

### Signs You Need Tuning

- **Large oscillations (±10°F+)**: Increase PB to 80 or 100
- **Won't reach setpoint (steady 5°F+ low)**: Decrease Ti to 150 or 120
- **Overshoots by 10°F then slowly settles**: Increase Ti to 200 or 220
- **Very slow response to lid opening**: Decrease PB to 50, decrease Td to 35

### Persistent Integral Term (NVS)

The PID integral term is saved to non-volatile storage (NVS) using the ESP32 `Preferences` library. This gives the PID a "warm start" on subsequent cooks, reaching steady-state faster.

**How it works:**
- When leaving RUNNING state (stop, cooldown, shutdown, or error), the integral value and setpoint are saved to NVS
- During RUNNING, a periodic save occurs every 5 minutes (handles unexpected power loss)
- When transitioning STARTUP → RUNNING, the saved integral is restored if the saved setpoint is within ±20°F of the current setpoint
- If setpoints differ too much (e.g., saved at 225°F, now cooking at 300°F), the integral starts fresh at 0.0
- First boot with no saved data behaves identically to before (integral = 0.0)

**Configuration** (`config.h`):
```cpp
#define ENABLE_PID_PERSISTENCE     true     // Save/restore integral across sessions
#define PID_SETPOINT_TOLERANCE     20.0     // °F - only restore if setpoint within this range
#define PID_SAVE_INTERVAL          300000   // ms (5 min) - periodic save during RUNNING
```

**NVS namespace**: `"smoker"`, keys: `"integral"` (float), `"setpoint"` (float)

**Serial log messages:**
- `[PID] Restored integral=X from NVS (saved@Y, current@Z, diff=D)` — successful restore
- `[PID] Discarding saved integral (setpoint diff=X > tolerance=Y)` — setpoint too different
- `[PID] No saved integral in NVS - starting at 0.0` — first boot or no saved data
- `[PID] Saved integral=X setpoint=Y to NVS` — save on state transition or periodic

## Common Customizations

### Change Default Target Temperature
Edit `main.cpp`, search for `_setpoint = 225.0;` (around line 50-60 in globals)

### Adjust PID Control (Tighter/Looser Control)
Edit `config.h`: Increase `PID_PROPORTIONAL_BAND` for more stable but slower control, decrease for tighter but potentially more oscillation-prone control. See PID Tuning section above.

### Change GPIO Pins
Edit `config.h`: All `PIN_*` defines at the top

### WiFi / MQTT / OTA / Syslog Credentials
Edit `include/secrets.h` (copy from `secrets.h.example` if it doesn't exist):
- `WIFI_SSID`, `WIFI_PASS` — WiFi station credentials
- `WIFI_AP_SSID`, `WIFI_AP_PASS` — Fallback AP credentials
- `MQTT_BROKER_HOST`, `MQTT_USERNAME`, `MQTT_PASSWORD` — MQTT broker
- `OTA_PASSWORD` — Over-the-air update password
- `SYSLOG_SERVER` — Remote logging server IP

**Never edit credentials in `config.h`** — it's committed to git. All secrets go in `secrets.h` which is gitignored. Config.h has `#ifndef` fallbacks with placeholder values so the project compiles without `secrets.h`.

For OTA uploads via PlatformIO, set the `OTA_PASSWORD` environment variable:
```bash
OTA_PASSWORD=your-password pio run --target upload --upload-port YOUR-DEVICE-IP
```

### Syslog Remote Logging
Edit `secrets.h` for `SYSLOG_SERVER`, edit `config.h` for non-secret settings:
- `ENABLE_SYSLOG` - Set to `true` to enable remote logging
- `SYSLOG_SERVER` - IP address of syslog server (set in secrets.h)
- `SYSLOG_PORT` - UDP port (default: 514)
- `SYSLOG_DEVICE_NAME` - Hostname shown in logs
- `SYSLOG_APP_NAME` - Application name in logs

## Development Workflow

### Build & Upload
```bash
pio run                    # Build firmware
pio run --target upload    # Upload to ESP32
pio device monitor         # Monitor serial output (115200 baud)
```

### Debugging
Enable `ENABLE_SERIAL_DEBUG = true` in config.h for verbose logging

### File System Upload (if needed)
```bash
pio run --target uploadfs  # Upload data/www/ to SPIFFS/LittleFS
```

### OTA (Over-The-Air) Updates
```bash
# Upload firmware via OTA (no serial cable needed)
pio run --target upload --upload-port <device_ip>

# Upload filesystem via OTA
pio run --target uploadfs --upload-port <device_ip>
```

**OTA Configuration** (platformio.ini):
- Hostname: `esp32-smoker`
- Password: `YOUR-OTA-PASSWORD` (configured in upload_flags)
- During OTA update, system automatically shuts down for safety

### HTTP OTA (Pull-Based Updates from GitHub)
The ESP32 periodically checks GitHub Releases for new firmware versions and auto-updates when idle.

**How it works:**
1. GitHub Actions builds `firmware.bin` + `version.txt` on version tags (`v*`)
2. ESP32 fetches `version.txt` from latest release every 6 hours
3. Compares semver with `FIRMWARE_VERSION` in config.h
4. If newer and smoker is idle → downloads `firmware.bin` and flashes

**Configuration** (`config.h`):
```cpp
#define ENABLE_HTTP_OTA          true
#define HTTP_OTA_CHECK_INTERVAL  21600000UL  // 6 hours
#define HTTP_OTA_FAST_INTERVAL   60000UL     // 60s (dev/testing mode)
#define HTTP_OTA_BOOT_DELAY      60000UL     // 60s after boot
#define HTTP_OTA_URL_BASE        "https://github.com/briangunderson/ESP32Smoker/releases/latest/download"
```

**Web UI:** "Check for Updates" and "Fast OTA" toggle buttons in Debug panel, firmware version in Info card. Fast OTA switches the check interval from 6 hours to 60 seconds for active development.

**API Endpoints:**
- `GET /api/version` — Current/latest version, update status, fastCheck flag
- `POST /api/update/check` — Manually trigger version check
- `POST /api/update/apply` — Install available update (smoker must be idle)
- `POST /api/update/fast` — Toggle fast OTA check interval (FormData: `enabled=true/false`)

**Release Workflow:**
1. Bump `FIRMWARE_VERSION` in config.h
2. Commit and push to main
3. `git tag v1.x.0 && git push origin v1.x.0`
4. GitHub Actions creates release with firmware.bin + version.txt
5. ESP32 auto-detects within 6 hours (or use web UI)

**Key files:** `include/http_ota.h`, `src/http_ota.cpp`, `.github/workflows/build.yml`

**Required GitHub secrets:** `WIFI_SSID`, `WIFI_PASS`, `WIFI_AP_PASS`, `MQTT_BROKER_HOST`, `MQTT_USERNAME`, `MQTT_PASSWORD`, `OTA_PASSWORD`, `SYSLOG_SERVER`

## Remote Logging with Syslog

The system supports sending logs to a remote syslog server (RFC 5424) over UDP. This allows you to monitor the system remotely without needing a serial connection.

### Configuration

Edit [config.h](include/config.h):
```cpp
#define ENABLE_SYSLOG        true                // Enable/disable syslog
#define SYSLOG_SERVER        "192.168.1.11"      // Syslog server IP
#define SYSLOG_PORT          514                 // UDP port (standard: 514)
#define SYSLOG_DEVICE_NAME   "ESP32Smoker"       // Device hostname
#define SYSLOG_APP_NAME      "smoker"            // App name in logs
```

### What Gets Logged

The system automatically logs:
- **Initialization events** - Startup, hardware initialization
- **Status updates** - Temperature, setpoint, relay states (every 10 seconds)
- **State changes** - State machine transitions
- **Errors** - Sensor errors, safety shutdowns

### Viewing Logs

Logs appear with the format:
```
<priority>1 timestamp hostname smoker - - - [TAG] message
```

Example:
```
<14>1 2026-01-29T10:30:00Z ESP32Smoker smoker - - - [STATUS] Temp: 225.0°F | Setpoint: 225.0°F | State: Running | Auger: ON | Fan: ON
```

### Syslog Priority Levels

The system uses standard RFC 5424 priority levels:
- **LOG_INFO (6)**: Normal operations, status updates
- **LOG_WARNING (4)**: Warnings (not yet used)
- **LOG_ERR (3)**: Errors, safety shutdowns (not yet used)
- **LOG_DEBUG (7)**: Debug messages (not yet used)

### Enterprise Logging Infrastructure

A complete Cribl/Elasticsearch/Kibana logging stack is available in `logging/`:

**Key Features:**
- **Cribl Stream**: Receives syslog on UDP port 9514, routes to Elasticsearch
- **Elasticsearch**: Stores logs in time-based indices (`esp32-logs-YYYY.MM.DD`)
- **Kibana Dashboard**: Real-time monitoring with temperature charts, PID performance, error tracking
- **Automated Alerts**: 4 watchers for critical events (ERROR state, temp out of range, sensor failures, high error rate)
- **Intelligent Parsing**: Extracts temperature, setpoint, PID values, state, relay status from log messages

**Quick Setup:**
```bash
cd logging/scripts
./setup-elasticsearch.sh  # Configure Elasticsearch
./setup-kibana.sh         # Configure Kibana + dashboard
./setup-watchers.sh       # Configure alerts
./test-logging.sh         # Verify pipeline works
```

**Access Points:**
- Cribl: http://YOUR-LOGGING-SERVER-IP:9000/ (user: claude)
- Kibana Dashboard: http://YOUR-LOGGING-SERVER-IP:5601/app/dashboards
- Elasticsearch: http://YOUR-LOGGING-SERVER-IP:9200/

**Dashboard Features:**
- Temperature vs setpoint timeline
- PID controller performance (P, I, D, Output)
- State transition history
- Error rate monitoring (15-minute window)
- Recent errors table
- Log level distribution

**Reusability:**
The infrastructure is designed to be modular and reusable for other ESP32 projects. See `logging/README.md` for detailed documentation on adapting it for different projects.

**Documentation:**
- `logging/README.md` - Complete guide (architecture, setup, troubleshooting, reuse)
- `logging/QUICK_REFERENCE.md` - Quick commands and URLs
- `logging/cribl/README.md` - Cribl configuration steps
- `logging/elasticsearch/` - All ES configs (templates, pipelines, watchers)
- `logging/kibana/` - All Kibana configs (visualizations, dashboard)

## Debug/Testing Features

The web interface includes a collapsible Debug/Testing panel with tools for hardware testing and troubleshooting.

### Debug Mode
**IMPORTANT**: When debug mode is enabled:
- Automatic temperature control is **disabled**
- State machine stops processing
- All relays are automatically turned off when entering debug mode
- You have full manual control of all relays and temperature reading

### Manual Relay Control
Control individual relays directly from the web interface or API:
- **Auger**: Turn on/off (safety interlock still enforced - requires fan running)
- **Fan**: Turn on/off
- **Igniter**: Turn on/off

Safety interlocks remain active even in debug mode to prevent unsafe operations.

### Temperature Override
Override the actual sensor reading with a test value:
- Set any temperature value (e.g., 70°F for testing cold start, 225°F for testing running mode)
- Useful for testing control logic without physical temperature changes
- Clear override to resume reading from actual MAX31865 sensor

### Using Debug Features
1. Open web interface at http://device_ip
2. Click on "🔧 Debug / Testing" to expand the panel
3. Click "Enable Debug Mode" button
4. Use the relay control buttons to manually test hardware
5. Enter a temperature value and click "Set Override" to test with simulated temperature
6. Click "Disable Debug Mode" when done testing to resume automatic control

**Command Line Testing**:
```bash
# Enable debug mode
curl -X POST -F "enabled=true" http://device_ip/api/debug/mode

# Set temperature override to 70°F
curl -X POST -F "temp=70" http://device_ip/api/debug/temp

# Turn on fan manually
curl -X POST -F "relay=fan" -F "state=true" http://device_ip/api/debug/relay

# Turn on auger (fan must be running first due to safety interlock)
curl -X POST -F "relay=auger" -F "state=true" http://device_ip/api/debug/relay

# Clear temperature override
curl -X DELETE http://device_ip/api/debug/temp

# Disable debug mode
curl -X POST -F "enabled=false" http://device_ip/api/debug/mode
```

## Important Notes

### Safety Interlocks
In `relay_control.cpp`:
- `setAuger(true)` will FAIL if fan is not running
- This prevents unburned pellets from accumulating (fire hazard)
- Always check return value or state after relay commands

### Sensor Reading
MAX31865 is read every 100ms via SPI in `max31865.cpp`:
- Uses Callendar-Van Dusen equation for accurate RTD conversion
- Configured for PT1000 sensor (1000Ω at 0°C) with 4.3kΩ reference resistor
- Supports 2-wire, 3-wire, or 4-wire RTD configurations (default: 3-wire)
- Fault detection for open circuit, short circuit, etc.

### Web Interface (PROGMEM Served)
Web files are embedded in firmware via PROGMEM (see `include/web_content.h`):
- **No filesystem required** - HTML/CSS/JS compiled into flash
- Source files in `data/www/` are for editing; `web_content.h` is generated from them
- After editing web files, regenerate `web_content.h` (raw string literals wrapping each file)
- Frontend polls `/api/status` every 2 seconds
- JS uses FormData for POST requests (matches ESPAsyncWebServer's body param parsing)
- **Important**: The board's partition table has `ffat` (FAT) partition type, not `spiffs`. LittleFS/SPIFFS filesystem uploads via OTA will silently fail. Use PROGMEM embedding instead.

### MQTT Authentication
MQTT broker connection uses username/password authentication:
- Credentials configured in `config.h`: `MQTT_USERNAME` and `MQTT_PASSWORD`
- Connection is authenticated in `mqtt_client.cpp` reconnect() method
- Make sure your MQTT broker has these credentials configured

## Known Limitations / Future Enhancements

### Implemented Features
- ✅ OTA (Over-The-Air) firmware and filesystem updates
- ✅ HTTP OTA (Pull-based updates from GitHub Releases)
- ✅ GitHub Actions CI/CD (build on push, release on tag)
- ✅ Debug mode with manual relay control
- ✅ Temperature override for testing
- ✅ MQTT authentication
- ✅ PROGMEM-embedded web interface (no filesystem dependency)
- ✅ PID control with Proportional Band method (from PiSmoker)
- ✅ Syslog remote logging
- ✅ Temperature history graph in web UI

### Not Yet Implemented
- Full persistent configuration storage (PID integral persists, but other settings reset on reboot)
- Temperature history logging
- Multiple temperature probes
- Data charts/graphs in web UI
- Reignite logic (automatic recovery if fire dies)
- Lid-open detection

### Current Behavior
- PID integral term persists across reboots (NVS); other settings reset on reboot
- Single temperature probe only
- Manual WiFi configuration (no captive portal)
- Basic web UI (functional but not fancy)

## Testing Checklist

Before deploying to actual hardware:
1. [ ] Verify all GPIO pin assignments match your hardware
2. [ ] Test sensor reading with actual MAX31865 + PT1000 probe
3. [ ] Verify TM1638 display shows temperature correctly
4. [ ] Test all 8 buttons on TM1638 module
5. [ ] Verify all 8 LEDs on TM1638 indicate correct status
6. [ ] Test each relay individually using debug mode
7. [ ] Verify auger interlock (auger should fail without fan even in debug mode)
8. [ ] Test debug mode: enable, control relays, override temperature, disable
9. [ ] Test OTA firmware update (verify system shuts down safely during update)
10. [ ] Test startup sequence with serial monitor
11. [ ] Test temperature control in safe environment
12. [ ] Test web interface from multiple devices
13. [ ] Test MQTT integration with Home Assistant
14. [ ] Test emergency stop scenarios
15. [ ] Verify thermal limits trigger correctly
16. [ ] Test physical buttons work in parallel with web interface

## Troubleshooting Quick Reference

### No Temperature Reading
- Check MAX31865 SPI wiring (CLK, MOSI, MISO, CS)
- Verify PT1000 probe connections (3-wire: RTD+, RTD-, RTDIN+)
- Ensure reference resistor matches config (4.3kΩ for PT1000)
- Check serial debug output for fault codes

### Display Not Working
- Verify TM1638 wiring (STB=GPIO25, CLK=GPIO26, DIO=GPIO27)
- Check power supply to TM1638 module (5V recommended)
- Monitor serial output for button press confirmations
- Look for test pattern on boot: "88888888" should flash for 0.5 seconds
- Check serial for: `[TM1638] Display initialized and ready`
- If compilation fails on TM16XX.cpp, the library patch may need reapplying

### Relays Not Switching
- Verify GPIO pins in config.h match your hardware
- Check relay module power supply
- Test relays manually with simple GPIO toggle

### WiFi Connection Failed
- Check SSID/password in config.h
- Try AP mode: connect to "ESP32Smoker" network
- Access http://192.168.4.1 in AP mode

### MQTT Not Connecting
- Verify broker IP/port in config.h
- Verify username/password in config.h match broker settings
- Check network connectivity (ping broker from ESP32's network)
- Check broker logs for authentication errors (error code 5 = auth failed)
- Serial monitor shows connection error codes: rc=-4 (timeout), rc=5 (auth failed)

## Library Dependencies

The project uses the following libraries (managed via PlatformIO):
- **ArduinoJson** (^6.20.0) - JSON parsing for configuration and API
- **ESPAsyncWebServer** (GitHub) - Asynchronous web server
- **AsyncTCP** (GitHub) - TCP library for async web server
- **PubSubClient** (^2.8) - MQTT client library
- **TM1638** (GitHub: rjbatista/tm1638-library) - Display and button interface
- **Syslog** (^2.0.0) - RFC 5424 syslog client for remote logging

All dependencies are automatically installed by PlatformIO on first build.

## Safety Reminders for AI Assistants

When making changes:
- **Always** check safety implications (especially relay control)
- Maintain the interlock: auger requires fan
- Test temperature limits after any control changes
- Consider logging/debugging when modifying core control logic

Historical bug fixes and hardware lessons are captured in the project's auto-memory (MEMORY.md).
