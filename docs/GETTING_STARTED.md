# Getting Started Guide

## Quick Start (5 minutes)

### 1. Install PlatformIO
- Install VS Code
- Install PlatformIO extension (`platformio.ideplus.vscode-ide`)
- Restart VS Code

### 2. Open Project
```bash
cd d:\localrepos\ESP32Smoker
code .
```

### 3. Build Project
- Click "Build" in PlatformIO toolbar
- Or: `pio run`

### 4. Upload to ESP32
1. Connect ESP32 via USB
2. Click "Upload" in PlatformIO toolbar
3. Or: `pio run --target upload`

### 5. Monitor Serial Output
1. Click "Serial Monitor" in PlatformIO toolbar
2. Or: `pio device monitor --baud 115200`

### 6. Access Web Interface
- Open browser
- Go to `http://192.168.4.1`
- Default AP: `ESP32Smoker` / `smoker12345`

## Hardware Setup

### Parts List
- ESP32 DevKit board ($12-15)
- MAX31865 RTD converter ($15-20)
- PT1000 RTD temperature probe ($20-40)
- TM1638 LED & button module ($5-8)
- 3-relay module ($5-10)
- USB power supply (5V 2A minimum)
- Jumper wires, connectors
- Enclosure (optional)

### Wiring Quick Reference
See [hardware/WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md) for complete wiring guide.

```
ESP32 GPIO 18 → MAX31865 CLK
ESP32 GPIO 23 → MAX31865 MOSI
ESP32 GPIO 19 → MAX31865 MISO
ESP32 GPIO 5  → MAX31865 CS

ESP32 GPIO 12 → Relay 1 IN (Auger)
ESP32 GPIO 13 → Relay 2 IN (Fan)
ESP32 GPIO 14 → Relay 3 IN (Igniter)

ESP32 GPIO 25 → TM1638 STB
ESP32 GPIO 26 → TM1638 CLK
ESP32 GPIO 27 → TM1638 DIO

PT1000 Probe → MAX31865 RTD pins
```

## Configuration

### Basic Configuration (config.h)

**Temperature Limits:**
```cpp
#define TEMP_MIN_SETPOINT 150    // Minimum
#define TEMP_MAX_SETPOINT 500    // Maximum
```

**Hysteresis Band:**
```cpp
#define TEMP_HYSTERESIS_BAND 10  // ±5°F from setpoint
```

**WiFi AP Mode (default):**
```cpp
#define WIFI_AP_SSID "ESP32Smoker"
#define WIFI_AP_PASS "smoker12345"
```

**MQTT (for Home Assistant):**
```cpp
#define MQTT_BROKER_HOST "192.168.1.100"
#define MQTT_ROOT_TOPIC "home/smoker"
```

## Common Tasks

### Change Default Temperature
Edit `config.h`:
```cpp
#define TEMP_MIN_SETPOINT 175    // Min
#define TEMP_MAX_SETPOINT 500    // Max
```

### Configure WiFi
Edit `config.h`:
```cpp
#define WIFI_SSID "Your-Network"
#define WIFI_PASS "Your-Password"
```

### Adjust Temperature Control Speed
Edit `config.h`:
```cpp
#define TEMP_CONTROL_INTERVAL 1000  // Check every 1 second (default: 2000)
#define TEMP_HYSTERESIS_BAND 5      // Tighter control (default: 10)
```

### Enable Debug Output
Edit `config.h`:
```cpp
#define ENABLE_SERIAL_DEBUG true
```

Then monitor serial output at 115200 baud.

### Change GPIO Pins
Edit `config.h` - all pins defined at top:
```cpp
#define PIN_MAX31865_CS    5
#define PIN_RELAY_AUGER    12
#define PIN_RELAY_FAN      13
#define PIN_RELAY_IGNITER  14
```

## Testing Checklist

### Hardware
- [ ] ESP32 powers on (blue LED)
- [ ] Serial output visible at 115200 baud
- [ ] MAX31865 responds (check serial for "initialized")
- [ ] All relays click when toggled

### WiFi
- [ ] Can see "ESP32Smoker" network
- [ ] Can connect with password "smoker12345"
- [ ] Can ping 192.168.4.1

### Web Interface
- [ ] Can access http://192.168.4.1
- [ ] Dashboard loads without errors
- [ ] Temperature display updates
- [ ] Buttons are clickable

### Control
- [ ] Start button → relays activate → State shows "Starting"
- [ ] Temperature increases
- [ ] Stop button → auger stops → state shows "Cooling Down"
- [ ] Shutdown button → all relays off

### MQTT (if configured)
- [ ] Subscribe to `home/smoker/sensor/temperature`
- [ ] See temperature values publishing every 5 seconds

## Troubleshooting

### ESP32 Won't Upload
1. Check USB cable (data cable, not charge-only)
2. Verify COM port in PlatformIO
3. Hold BOOT button while uploading
4. Try different USB port

### No Serial Output
1. Check baud rate (115200)
2. Verify USB driver installed
3. Try different USB cable
4. Check Tools → Port in Arduino IDE

### Can't Connect to WiFi
1. Verify SSID and password in config.h
2. Check if WiFi network exists and is 2.4 GHz
3. Use AP mode by leaving WIFI_SSID empty
4. Check antenna connection on ESP32

### Temperature Reads Crazy Values
1. Verify MAX31865 SPI wiring
2. Check RTD probe connection
3. Verify RTD is correct type (PT1000) and reference resistor is 4.3kΩ
4. If using PT100, update config.h resistor values accordingly
5. Try moving RTD to different connector

### Relays Don't Click
1. Check relay module power supply
2. Verify GPIO pins in config.h
3. Test with digitalWrite directly in code
4. Check for bad solder joints

### MQTT Not Connecting
1. Verify broker IP and port in config.h
2. Check network connectivity
3. Verify MQTT broker is running
4. Check firewall settings

## Next Steps

### Learn the System
1. Read [README.md](README.md) for overview
2. Check [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system design
3. Review [docs/API.md](docs/API.md) for web interface

### Customize
1. Adjust temperature limits for your smoker
2. Configure WiFi/MQTT for your network
3. Customize web interface colors/layout
4. Add sensors or features as needed

### Deploy
1. Test all functionality thoroughly
2. Mount in weatherproof enclosure
3. Verify all wiring and connections
4. Run in standalone mode (no computer)
5. Test Home Assistant integration

### Extend
1. Add multiple temperature sensors
2. Implement data logging to SD card
3. Add over-the-air firmware updates
4. Implement PID control algorithm
5. Create mobile app

## Resources

### Files to Read
- [README.md](README.md) - Main documentation
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - System design
- [docs/API.md](docs/API.md) - API reference
- [hardware/WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md) - Wiring guide

### Code to Study
- `src/main.cpp` - Application entry point
- `include/temperature_control.h` - Control logic
- `src/max31865.cpp` - Temperature sensor driver
- `data/www/script.js` - Web interface logic

### Online Resources
- ESP32 Documentation: https://docs.espressif.com/projects/esp-idf/
- PlatformIO Docs: https://docs.platformio.org/
- Arduino Framework: https://www.arduino.cc/reference/en/
- Home Assistant MQTT: https://www.home-assistant.io/integrations/mqtt/

## Support

### Debugging
1. Enable `ENABLE_SERIAL_DEBUG = true` in config.h
2. Monitor serial output with baud 115200
3. Check logs for error messages
4. Verify all hardware connections

### Common Log Messages
```
[SETUP] MAX31865 sensor initialized        → MAX31865 working
[TEMP] Temperature controller initialized  → Control system ready
[WIFI] Connected! IP: 192.168.1.100        → WiFi connected
[MQTT] Connected as esp32-smoker           → MQTT ready
[STATUS] Temp: 220.3°F | State: Running    → System operating normally
```

---

**Good luck with your smoker controller! 🔥**

For detailed guides, see the docs/ folder.
