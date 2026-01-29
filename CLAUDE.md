# ESP32 Wood Pellet Smoker Controller - Context for Claude

## Project Overview
This is a complete, production-ready wood pellet smoker controller built on ESP32 with Arduino framework. It features real-time temperature control, web interface, and Home Assistant integration via MQTT.

**Status**: Ready for hardware integration with OTA and debug features
**Version**: 1.1.0
**Last Build**: January 28, 2026

## Quick Facts
- **Platform**: ESP32 (PlatformIO + Arduino Framework)
- **Language**: C++ for firmware, HTML/CSS/JS for web interface
- **Temperature Sensor**: MAX31865 RTD (PT1000 probe via SPI)
- **Display**: TM1638 (dual 7-segment displays, 8 buttons, 8 LEDs)
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
- `tm1638_display.cpp` - Physical display and button interface

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

TM1638 Display:
  STB  = GPIO 25 (Strobe)
  CLK  = GPIO 26 (Clock)
  DIO  = GPIO 27 (Data I/O)

Status:
  LED = GPIO 2 (Built-in)
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
8. **Reserved** - Available for future use

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
- MQTT broker: Configure host/port/credentials in config.h
  - Authentication enabled with username/password

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
Edit `config.h`:
- `MQTT_BROKER_HOST` and `MQTT_BROKER_PORT`
- `MQTT_USERNAME` and `MQTT_PASSWORD` (authentication credentials)

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
- Password: `smoker2026` (configured in upload_flags)
- During OTA update, system automatically shuts down for safety

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
- Configured for PT1000 sensor (1000Ω at 0°C) with 4.3kΩ reference resistor
- Supports 2-wire, 3-wire, or 4-wire RTD configurations (default: 3-wire)
- Fault detection for open circuit, short circuit, etc.

### Web Interface Updates
Frontend polls `/api/status` every 2 seconds (see `script.js`)
- To change update rate: modify `setInterval(updateStatus, 2000)` in script.js

### MQTT Authentication
MQTT broker connection uses username/password authentication:
- Credentials configured in `config.h`: `MQTT_USERNAME` and `MQTT_PASSWORD`
- Connection is authenticated in `mqtt_client.cpp` reconnect() method
- Make sure your MQTT broker has these credentials configured

## Known Limitations / Future Enhancements

### Implemented Features
- ✅ OTA (Over-The-Air) firmware and filesystem updates
- ✅ Debug mode with manual relay control
- ✅ Temperature override for testing
- ✅ MQTT authentication
- ✅ LittleFS filesystem for web interface

### Not Yet Implemented
- Persistent configuration storage (settings reset on reboot)
- Temperature history logging
- PID control algorithm (currently hysteresis only)
- Multiple temperature probes
- Data charts/graphs in web UI

### Current Behavior
- Settings reset on reboot (no persistence yet)
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

All dependencies are automatically installed by PlatformIO on first build.

## Recent Changes & Fixes

### January 29, 2026 - TM1638 Display Troubleshooting
**Issue**: TM1638 display showing only power LED, no segments or status LEDs

**Root Cause**: Library compilation issue and missing diagnostics

**Fixes Applied**:
1. **TM16XX Library Patch** - Fixed `min()` function type mismatch in [.pio/libdeps/esp32dev/tm1638-library/TM16XX.cpp](src/tm1638_display.cpp)
   - Changed `min(7, intensity)` to `min((byte)7, intensity)` to resolve C++ template deduction error

2. **Enhanced TM1638 Initialization** ([tm1638_display.cpp](src/tm1638_display.cpp))
   - Added diagnostic serial output showing pin assignments
   - Added 500ms startup test pattern ("88888888" + all LEDs)
   - Added explicit intensity parameter (7 = maximum brightness)

3. **Invalid Temperature Handling** ([tm1638_display.cpp:58-62](src/tm1638_display.cpp#L58-L62))
   - Added NaN/infinity checks in `formatTemperature()`
   - Invalid temps now display as "----" instead of garbage

4. **Display Update Logging** ([tm1638_display.cpp:38-43](src/tm1638_display.cpp#L38-L43))
   - Added periodic debug output showing what's being displayed
   - Helps verify data is being sent to display

5. **Button Handler Fix** ([main.cpp:121](src/main.cpp#L121))
   - Changed `controller->start()` to `controller->startSmoking(controller->getSetpoint())`
   - Fixed compilation error due to method name mismatch

**Verification**: Display now shows current temp (left) and setpoint (right), status LEDs work, buttons functional

## Project Context for AI Assistants

This project was initially built on January 28, 2026, with hardware enhancements added on January 29, 2026. The codebase is well-structured with:
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
