# Quick Reference Card

## 🚀 Quick Start

```bash
# 1. Build firmware
pio run

# 2. Upload to ESP32
pio run --target upload

# 3. Monitor serial (Ctrl+C to quit)
pio device monitor --baud 115200

# 4. Access web interface
# Browser: http://192.168.4.1
# Default WiFi: "ESP32Smoker" / "your-ap-password"
```

## 📍 Pin Assignments

| Function | GPIO | Usage |
|----------|------|-------|
| SPI CLK | 18 | MAX31865 temperature |
| SPI MOSI | 23 | Temperature sensor |
| SPI MISO | 19 | Temperature sensor |
| SPI CS | 5 | Temperature sensor |
| Relay 1 | 12 | Pellet auger |
| Relay 2 | 13 | Combustion fan |
| Relay 3 | 14 | Hot rod igniter |
| TM1638 STB | 25 | Display strobe |
| TM1638 CLK | 26 | Display clock |
| TM1638 DIO | 27 | Display data |
| LED | 2 | Status indicator |

## 🌡️ Temperature Control

| Setting | Value | Configurable |
|---------|-------|--------------|
| Min setpoint | 150°F | Yes (config.h) |
| Max setpoint | 350°F | Yes (config.h) |
| Hysteresis band | 10°F (±5°F) | Yes (config.h) |
| Control interval | 2000ms | Yes (config.h) |
| Min safe | 50°F | Yes (config.h) |
| Max safe | 500°F | Yes (config.h) |

## 🔌 Relay Behavior

### Auger (Pellet Feeder)
- **ON**: Feeding pellets
- **OFF**: No feeding
- **Interlock**: Won't turn ON unless fan is running

### Fan (Air/Oxygen)
- **ON**: Provides combustion air
- **OFF**: No air supply
- **Safety**: Always keep on during RUNNING state

### Igniter (Hot Rod)
- **ON**: Heating during startup only
- **OFF**: Normal operation
- **Startup**: 60 second preheat before fan starts

## 🎮 Physical Controls (TM1638 Display)

### Display
- **Left 4 digits**: Current temperature (°F)
- **Right 4 digits**: Target setpoint (°F)

### Status LEDs (Left to Right)
1. **Auger** - Pellet feeder running
2. **Fan** - Combustion fan running
3. **Igniter** - Hot rod active
4. **WiFi** - Connected to network
5. **MQTT** - Connected to broker
6. **Error** - System fault
7. **Running** - Smoking in progress
8. **Heartbeat** - Blinks every second (system alive)

### Buttons (Left to Right)
1. **Start** - Begin smoking
2. **End Cook** - Stop and cooldown
3. **Temp ▲** - Increase by 5°F
4. **Temp ▼** - Decrease by 5°F
5. **Mode** - Reserved

## 🎯 State Machine

```
START     →  STARTUP  →  RUNNING  →  COOLDOWN  →  STOPPED   →  IDLE
(0-60s)      (0-180s)   (normal)    (temp>100)   (relays off)
 preheat      heating    control     safety cool  final state
   + ─────────────────── ERROR STATE ────────────────┘
     (on sensor error or temp fault)
```

## 🌐 Web Interface URLs

```
Dashboard:   http://192.168.4.1/
API Status:  http://192.168.4.1/api/status
```

## 📡 REST API Quick Ref

```bash
# Get status
curl http://192.168.4.1/api/status

# Start smoking at 225°F
curl -X POST http://192.168.4.1/api/start \
  -H "Content-Type: application/json" \
  -d '{"temp": 225}'

# End Cook (cooldown)
curl -X POST http://192.168.4.1/api/stop

# Emergency Stop
curl -X POST http://192.168.4.1/api/shutdown

# Set target temp
curl -X POST http://192.168.4.1/api/setpoint \
  -H "Content-Type: application/json" \
  -d '{"temp": 250}'
```

## 📊 Status Response Fields

```json
{
  "temp": 215.3,        // Current temperature (°F)
  "setpoint": 225.0,    // Target temperature (°F)
  "state": "Running",   // Current state
  "auger": true,        // Auger relay ON/OFF
  "fan": true,          // Fan relay ON/OFF
  "igniter": false,     // Igniter relay ON/OFF
  "runtime": 3600000,   // Time in current state (ms)
  "errors": 0           // Consecutive sensor errors
}
```

## 🏠 Home Assistant MQTT Topics

```
Sensors (subscribed):
home/smoker/sensor/temperature  → Current temp (°F)
home/smoker/sensor/setpoint     → Target temp (°F)
home/smoker/sensor/state        → System state
home/smoker/sensor/auger        → "on" or "off"
home/smoker/sensor/fan          → "on" or "off"
home/smoker/sensor/igniter      → "on" or "off"

Commands (published):
home/smoker/command/start       → float (temp)
home/smoker/command/stop        → (any)
home/smoker/command/setpoint    → float (temp)
```

## ⚙️ Configuration Cheat Sheet

```cpp
// RTD Sensor (PT1000)
#define MAX31865_RTD_RESISTANCE_AT_0  1000.0  // PT1000 = 1000Ω
#define MAX31865_REFERENCE_RESISTANCE 4300.0  // 4.3kΩ reference

// Temperature Limits (°F)
#define TEMP_MIN_SETPOINT      150
#define TEMP_MAX_SETPOINT      350

// PID Gains (tune for your smoker)
#define PID_KP                 4.0   // Proportional gain
#define PID_KI                 0.01  // Integral gain
#define PID_KD                 100.0 // Derivative gain
#define AUGER_CYCLE_TIME       15000 // Auger PWM window (ms)

// Timing (milliseconds)
#define IGNITER_PREHEAT_TIME   60000 // 60 seconds
#define FAN_STARTUP_DELAY      5000  // 5 seconds
#define STARTUP_TIMEOUT        180000 // 3 minutes max
#define TEMP_CONTROL_INTERVAL  2000  // Check every 2 sec

// Safety
#define TEMP_MAX_SAFE          500    // Absolute max
#define TEMP_MIN_SAFE          50     // Absolute min
#define SENSOR_ERROR_THRESHOLD 3      // Errors before shutdown

// WiFi
#define WIFI_SSID              ""     // Empty = AP mode
#define WIFI_PASS              ""
#define WIFI_AP_SSID           "ESP32Smoker"
#define WIFI_AP_PASS           "your-ap-password"

// MQTT
#define MQTT_BROKER_HOST       "192.168.1.100"
#define MQTT_BROKER_PORT       1883
#define MQTT_ROOT_TOPIC        "home/smoker"

// Debug
#define ENABLE_SERIAL_DEBUG    true
#define SERIAL_BAUD_RATE       115200
```

## 🐛 Troubleshooting Quick Tips

| Problem | Solution |
|---------|----------|
| Won't upload | Hold BOOT button, check COM port |
| No serial output | Check baud 115200, USB driver |
| Can't connect WiFi | Use AP mode or check network |
| Temp reads crazy | Check MAX31865 SPI, verify PT1000 |
| Display blank | Check TM1638 wiring, 5V power |
| Buttons not working | Check serial for button presses |
| Relays don't click | Check power supply, GPIO pins |
| MQTT offline | Check broker IP and port |
| Web page slow | Check WiFi signal strength |

## 📚 Documentation Map

```
📖 Start here:
   └─ docs/GETTING_STARTED.md ......... 5-minute quickstart
      
📋 How to use:
   ├─ README.md ....................... Features & overview
   ├─ docs/API.md ..................... REST endpoint reference
   └─ hardware/WIRING_DIAGRAM.md ...... Hardware connections
      
🏗️ How it works:
   ├─ docs/ARCHITECTURE.md ............ System design & algorithms
   └─ BUILD_SUMMARY.md ................ What was built
      
💾 Configuration:
   ├─ include/config.h ................ All settings in one file
   ├─ src/temperature_control.cpp ..... Control algorithm
   └─ src/main.cpp .................... Entry point
      
🌐 Home Assistant:
   └─ docs/API.md (MQTT Topics) ....... Integration setup
```

## 🔧 Common Customizations

**Tighter control (faster response):**
```cpp
#define TEMP_HYSTERESIS_BAND 5       // ±2.5°F
#define TEMP_CONTROL_INTERVAL 1000   // Every 1 sec
```

**More stable (fewer oscillations):**
```cpp
#define TEMP_HYSTERESIS_BAND 20      // ±10°F
#define TEMP_CONTROL_INTERVAL 3000   // Every 3 sec
```

**Faster startup (careful!):**
```cpp
#define IGNITER_PREHEAT_TIME 30000   // 30 sec preheat
#define FAN_STARTUP_DELAY 2000       // 2 sec delay
```

**Different temperature range:**
```cpp
#define TEMP_MIN_SETPOINT 200  // Raise minimum
#define TEMP_MAX_SETPOINT 300  // Lower maximum
```

## ✅ Pre-Deployment Checklist

- [ ] Code builds without errors (`pio run`)
- [ ] Uploads successfully to ESP32
- [ ] Serial output visible at startup
- [ ] TM1638 display shows temperature
- [ ] All 8 LEDs illuminate correctly
- [ ] All physical buttons respond
- [ ] WiFi AP visible ("ESP32Smoker")
- [ ] Can connect to web interface
- [ ] All relays click when toggled
- [ ] Temperature sensor reads reasonable values (PT1000)
- [ ] Start/Stop/Shutdown buttons work (web & physical)
- [ ] Temperature slider adjusts target
- [ ] MQTT publishes (if configured)
- [ ] Home Assistant receives updates

## 📱 Web Dashboard Features

| Feature | Purpose |
|---------|---------|
| Large temperature display | Easy to read from distance |
| Target temperature slider | Quick adjustments (150-350°F) |
| Start button | Begin smoking session |
| End Cook button | Initiate cooldown |
| Emergency Stop button | Immediately turn off all relays |
| State display | Know what system is doing |
| Relay status lights | See which motors are running |
| WiFi/MQTT indicators | Network status at a glance |

---

**📘 For detailed info, see [README.md](README.md) or [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md)**

**🔥 Happy smoking!**
