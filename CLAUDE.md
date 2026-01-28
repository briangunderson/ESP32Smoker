# ESP32 Wood Pellet Smoker Controller - Context for Claude

## Project Overview
This is a complete, production-ready wood pellet smoker controller built on ESP32 with Arduino framework. It features real-time temperature control, web interface, and Home Assistant integration via MQTT.

**Status**: Ready for hardware integration
**Version**: 1.0.0
**Last Build**: January 28, 2026

## Quick Facts
- **Platform**: ESP32 (PlatformIO + Arduino Framework)
- **Language**: C++ for firmware, HTML/CSS/JS for web interface
- **Temperature Sensor**: MAX31865 RTD (PT100 probe via SPI)
- **Control**: 3 relays (auger, fan, igniter) with safety interlocks
- **Connectivity**: WiFi (STA/AP modes) + Web API + MQTT
- **Lines of Code**: ~4000 total (~1200 firmware, ~950 web UI, ~1500 docs)

## Critical Files

### Configuration
- `include/config.h` - **ALL configuration in one place** (pins, temps, timings, WiFi, MQTT)

### Core Firmware (src/)
- `main.cpp` - Application entry point, initialization, main loop
- `temperature_control.cpp` - State machine & hysteresis control (THE BRAIN)
- `max31865.cpp` - RTD sensor driver (SPI communication)
- `relay_control.cpp` - GPIO relay control with safety interlocks
- `web_server.cpp` - Async HTTP server + REST API
- `mqtt_client.cpp` - Home Assistant integration

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
- `hardware/WIRING_DIAGRAM.md` - Complete wiring guide

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
4. Wait for temp to reach setpoint

**RUNNING mode**:
- Hysteresis control: ±5°F band around setpoint (default: 225°F)
- Auger cycles on/off to maintain temp
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
  CLK  = GPIO 18
  MOSI = GPIO 23
  MISO = GPIO 19
  CS   = GPIO 5

Relays:
  Auger   = GPIO 12
  Fan     = GPIO 13
  Igniter = GPIO 14

Status:
  LED = GPIO 2
```

## Key Configuration Parameters

### Temperature Control (config.h)
- `TEMP_MIN_SETPOINT`: 150°F (minimum allowed)
- `TEMP_MAX_SETPOINT`: 350°F (maximum allowed)
- `TEMP_HYSTERESIS_BAND`: 10°F (±5°F around setpoint)
- `TEMP_CONTROL_INTERVAL`: 2000ms (update rate)

### Timing (config.h)
- `IGNITER_PREHEAT_TIME`: 60000ms (1 minute)
- `FAN_STARTUP_DELAY`: 5000ms (5 seconds)
- `STARTUP_TIMEOUT`: 180000ms (3 minutes)

### Network (config.h)
- WiFi STA mode for configured network
- AP fallback mode: SSID "ESP32Smoker", password in config
- Web interface: http://device_ip or http://192.168.4.1 (AP mode)
- MQTT broker: Configure host/port in config.h

## REST API Endpoints

- `GET /api/status` - Current temp, setpoint, state, relay status
- `POST /api/start` - Start smoking (optional: `{"temp": 250}`)
- `POST /api/stop` - Stop feeding pellets, initiate cooldown
- `POST /api/shutdown` - Full system shutdown
- `POST /api/setpoint` - Update target temp: `{"temp": 225}`

## MQTT Topics

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

## Common Customizations

### Change Default Target Temperature
Edit `main.cpp`, search for `_setpoint = 225.0;` (around line 50-60 in globals)

### Adjust Hysteresis Band (Tighter/Looser Control)
Edit `config.h`: `#define TEMP_HYSTERESIS_BAND 10` (10 = ±5°F, 5 = ±2.5°F)

### Change GPIO Pins
Edit `config.h`: All `PIN_*` defines at the top

### WiFi Credentials
Edit `config.h`: `WIFI_SSID` and `WIFI_PASS` (or use AP mode)

### MQTT Broker
Edit `config.h`: `MQTT_BROKER_HOST` and `MQTT_BROKER_PORT`

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

## Important Notes

### Control Algorithm
The hysteresis control in `temperature_control.cpp` is the heart of the system:
- Lower bound = setpoint - (band / 2)
- Upper bound = setpoint + (band / 2)
- Temperature below lower → auger ON
- Temperature above upper → auger OFF
- Temperature in band → auger maintains state (prevents oscillation)

### Safety Interlocks
In `relay_control.cpp`:
- `setAuger(true)` will FAIL if fan is not running
- This prevents unburned pellets from accumulating (fire hazard)
- Always check return value or state after relay commands

### Sensor Reading
MAX31865 is read every 100ms via SPI in `max31865.cpp`:
- Uses Callendar-Van Dusen equation for accurate RTD conversion
- Supports 2-wire, 3-wire, or 4-wire RTD configurations
- Fault detection for open circuit, short circuit, etc.

### Web Interface Updates
Frontend polls `/api/status` every 2 seconds (see `script.js`)
- To change update rate: modify `setInterval(updateStatus, 2000)` in script.js

## Known Limitations / Future Enhancements

### Not Yet Implemented
- Persistent configuration storage (SPIFFS/LittleFS)
- Temperature history logging
- PID control algorithm (currently hysteresis only)
- Multiple temperature probes
- OTA (Over-The-Air) firmware updates
- Data charts/graphs in web UI

### Current Behavior
- Settings reset on reboot (no persistence yet)
- Single temperature probe only
- Manual WiFi configuration (no captive portal)
- Basic web UI (functional but not fancy)

## Testing Checklist

Before deploying to actual hardware:
1. [ ] Verify all GPIO pin assignments match your hardware
2. [ ] Test sensor reading with actual MAX31865 + PT100 probe
3. [ ] Test each relay individually
4. [ ] Verify auger interlock (auger should fail without fan)
5. [ ] Test startup sequence with serial monitor
6. [ ] Test temperature control in safe environment
7. [ ] Test web interface from multiple devices
8. [ ] Test MQTT integration with Home Assistant
9. [ ] Test emergency stop scenarios
10. [ ] Verify thermal limits trigger correctly

## Troubleshooting Quick Reference

### No Temperature Reading
- Check MAX31865 SPI wiring (CLK, MOSI, MISO, CS)
- Verify PT100 probe connections
- Check serial debug output for fault codes

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
- Check network connectivity (ping broker from ESP32's network)
- Check broker logs for connection errors

## Project Context for AI Assistants

This project was built in a single session on January 28, 2026. The codebase is well-structured with:
- Clear separation of concerns (each module in its own file)
- Comprehensive documentation at multiple levels
- Safety-first design with interlocks and fault handling
- Production-ready code (not a prototype)

When making changes:
- **Always** check safety implications (especially relay control)
- Maintain the interlock: auger requires fan
- Test temperature limits after any control changes
- Update relevant documentation when changing behavior
- Consider logging/debugging when modifying core control logic

The code prioritizes safety and reliability over advanced features. Keep this philosophy when extending functionality.

---

**For quick orientation**: Start with README.md, then review include/config.h, then read src/temperature_control.cpp to understand the control logic.
