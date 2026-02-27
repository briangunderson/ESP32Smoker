# ESP32 Wood Pellet Smoker Controller

A modern, fully-featured wood pellet smoker controller built on ESP32 with WiFi, web interface, and Home Assistant integration via MQTT.

## Features

- **Real-time Temperature Monitoring** via MAX31865 RTD sensor
- **3-Relay Control** for auger, fan, and igniter
- **PID Temperature Control** for precise, stable operation (based on PiSmoker)
- **Modern Web Dashboard** with responsive UI
- **Home Assistant Integration** via MQTT
- **WiFi Connectivity** with AP fallback mode
- **Remote Logging** via Syslog (RFC 5424) for network-based monitoring
- **Safety Features** including thermal limits and sensor error detection
- **Configuration Persistence** using LittleFS
- **Real-time Status Monitoring** and logging

## Hardware Requirements

### Microcontroller
- **ESP32 Development Board** (e.g., ESP32-DevKitC)
  - Dual-core 240 MHz CPU
  - 8+ MB Flash
  - WiFi 802.11 b/g/n

### Sensors
- **MAX31865 RTD-to-Digital Converter**
  - SPI interface
  - 3-wire, 2-wire, or 4-wire RTD support
  - PT1000 RTD probe (1000Ω at 0°C)

### Display & Controls
- **TM1638 LED & Button Module**
  - Dual 7-segment displays (current temp + setpoint)
  - 8 status LEDs (auger, fan, igniter, WiFi, MQTT, error, running)
  - 8 push buttons (start, stop, temp up/down, mode selection)

### Control Hardware
- **3-Relay Module** (5V or 12V)
  - Relay 1: Pellet Auger Motor
  - Relay 2: Combustion Fan
  - Relay 3: Hot Rod Igniter

### Power
- 5V USB or bench power supply for ESP32
- 12V or 120V AC power for relays (depending on configuration)

## Wiring Diagram

```
ESP32                          MAX31865          Relays
┌─────────────────────────────────────────────────────────┐
│                                                          │
│ GPIO 18 (CLK) ─────────────────► CLK              │
│ GPIO 23 (MOSI) ────────────────► MOSI             │
│ GPIO 19 (MISO) ◄────────────────── MISO           │
│ GPIO 5 (CS) ──────────────────► CS                │
│                                                          │
│ GPIO 12 ─────────────────────────────────────► Relay 1 (Auger)
│ GPIO 13 ─────────────────────────────────────► Relay 2 (Fan)
│ GPIO 14 ─────────────────────────────────────► Relay 3 (Igniter)
│                                                          │
│ GPIO 25 ─────────────────────────────────────► TM1638 STB
│ GPIO 26 ─────────────────────────────────────► TM1638 CLK
│ GPIO 27 ─────────────────────────────────────► TM1638 DIO
│                                                          │
│ GPIO 2 ──────────────────────────────────────► LED Status
│                                                          │
└─────────────────────────────────────────────────────────┘

MAX31865 to RTD Probe (3-wire PT1000 example):
  RTD+ (Pin 5) ─────► PT1000 Probe A
  RTD- (Pin 6) ─────► PT1000 Probe B
  RTDIN+ (Pin 1) ───► PT1000 Probe B
  RTDIN- (Pin 2) ───► GND
```

## Installation

### 1. Prerequisites
- PlatformIO installed in VS Code
- Python 3.7+ (for PlatformIO)
- ESP32 board support in PlatformIO

### 2. Clone & Setup
```bash
cd d:\localrepos\ESP32Smoker
```

### 3. Build and Upload
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200
```

## Configuration

### Main Configuration File: `include/config.h`

Key parameters to adjust:

```cpp
// GPIO Pin Assignments
#define PIN_MAX31865_CS    5      // Chip select pin
#define PIN_RELAY_AUGER    12     // Auger relay
#define PIN_RELAY_FAN      13     // Fan relay
#define PIN_RELAY_IGNITER  14     // Igniter relay

// Temperature Control
#define TEMP_MIN_SETPOINT      150    // Minimum allowed (°F)
#define TEMP_MAX_SETPOINT      350    // Maximum allowed (°F)
#define TEMP_HYSTERESIS_BAND   10     // ±5°F band (±10°F total)
#define TEMP_CONTROL_INTERVAL  2000   // Control update rate (ms)

// Startup/Shutdown
#define IGNITER_PREHEAT_TIME   60000  // Igniter warm-up (ms)
#define FAN_STARTUP_DELAY      5000   // Fan startup delay (ms)
#define STARTUP_TIMEOUT        180000 // Startup timeout (3 min)

// WiFi
#define WIFI_AP_SSID   "ESP32Smoker"  // Access point SSID
#define WIFI_AP_PASS   "your-ap-password"  // Access point password

// MQTT
#define MQTT_BROKER_HOST   "192.168.1.100"  // Broker IP
#define MQTT_BROKER_PORT   1883              // Broker port
#define MQTT_ROOT_TOPIC    "home/smoker"    // Root topic
```

## Usage

### Web Interface

1. **Connect to WiFi**
   - Default AP: `ESP32Smoker` / `your-ap-password`
   - Browse to `http://192.168.4.1` (or device IP)

2. **Main Controls**
   - **Temperature Display**: Shows current and target temperature
   - **Start Button**: Begins smoking at set temperature
   - **End Cook Button**: Stops feeding pellets, cools down
   - **Emergency Stop Button**: Immediately turns off all relays
   - **Temperature Slider**: Adjust target in 1°F increments

3. **Status Indicators**
   - **Green dots**: Connected systems (WiFi, MQTT, Sensor)
   - **Relay Status**: Shows auger, fan, igniter state
   - **System State**: Current operation mode

### Physical Controls (TM1638 Display)

The TM1638 module provides standalone control without requiring the web interface:

1. **Display**
   - **Left Display**: Current temperature in °F
   - **Right Display**: Target setpoint in °F

2. **Status LEDs** (left to right)
   - LED 1: Auger running
   - LED 2: Fan running
   - LED 3: Igniter active
   - LED 4: WiFi connected
   - LED 5: MQTT connected
   - LED 6: Error state
   - LED 7: Running/Smoking

3. **Button Controls** (left to right)
   - Button 1: **Start** - Begin smoking
   - Button 2: **Stop** - Stop and cooldown
   - Button 3: **Temp Up** - Increase setpoint by 5°F
   - Button 4: **Temp Down** - Decrease setpoint by 5°F
   - Button 5: **Mode** - Reserved for future use

### API Endpoints

All requests return JSON responses.

#### GET /api/status
Returns current system status.

```json
{
  "temp": 215.3,
  "setpoint": 225.0,
  "state": "Running",
  "auger": true,
  "fan": true,
  "igniter": false,
  "runtime": 3600000,
  "errors": 0
}
```

#### POST /api/start
Start smoking session.

**Parameters:**
- `temp` (optional): Target temperature (150-350°F, default 225°F)

**Request:**
```json
{ "temp": 250 }
```

#### POST /api/stop
End cook — stop feeding pellets, initiate cooldown.

#### POST /api/shutdown
Emergency stop — immediately turn off all relays.

#### POST /api/setpoint
Update target temperature.

**Parameters:**
- `temp`: Target temperature (150-350°F)

## Home Assistant Integration

### MQTT Topics

**Sensors** (published by ESP32):
```
home/smoker/sensor/temperature     → Current temp (°F)
home/smoker/sensor/setpoint        → Target temp (°F)
home/smoker/sensor/state           → System state
home/smoker/sensor/auger           → "on" or "off"
home/smoker/sensor/fan             → "on" or "off"
home/smoker/sensor/igniter         → "on" or "off"
```

**Commands** (subscribed by ESP32):
```
home/smoker/command/start          → Payload: float (temp)
home/smoker/command/stop           → (any payload)
home/smoker/command/setpoint       → Payload: float (temp)
```

### Home Assistant Configuration Example

Add to `configuration.yaml`:

```yaml
mqtt:
  broker: 192.168.1.100
  port: 1883

sensor:
  - platform: mqtt
    name: "Smoker Temperature"
    state_topic: "home/smoker/sensor/temperature"
    unit_of_measurement: "°F"
    device_class: temperature
    value_template: "{{ value | float(0) }}"

  - platform: mqtt
    name: "Smoker Setpoint"
    state_topic: "home/smoker/sensor/setpoint"
    unit_of_measurement: "°F"
    device_class: temperature

  - platform: mqtt
    name: "Smoker State"
    state_topic: "home/smoker/sensor/state"

binary_sensor:
  - platform: mqtt
    name: "Smoker Auger"
    state_topic: "home/smoker/sensor/auger"
    payload_on: "on"
    payload_off: "off"

  - platform: mqtt
    name: "Smoker Fan"
    state_topic: "home/smoker/sensor/fan"
    payload_on: "on"
    payload_off: "off"

automation:
  - alias: "Start Smoker at 225°F"
    trigger:
      platform: state
      entity_id: input_boolean.start_smoker
      to: "on"
    action:
      - service: mqtt.publish
        data:
          topic: "home/smoker/command/start"
          payload: "225"
      - service: input_boolean.turn_off
        entity_id: input_boolean.start_smoker

  - alias: "Stop Smoker"
    trigger:
      platform: state
      entity_id: input_boolean.stop_smoker
      to: "on"
    action:
      - service: mqtt.publish
        data:
          topic: "home/smoker/command/stop"
          payload: ""
      - service: input_boolean.turn_off
        entity_id: input_boolean.stop_smoker
```

## Temperature Control Logic

### State Machine

```
        ┌─────────┐
        │  IDLE   │
        └────┬────┘
             │ startSmoking()
             ▼
        ┌──────────┐
        │ STARTUP  │ (Pre-heat igniter → Start fan → Enable auger)
        └────┬─────┘
             │ Reached setpoint
             ▼
        ┌─────────┐
        │ RUNNING │ (Hysteresis control of auger)
        └────┬─────┘
             │ stop()
             ▼
        ┌────────────┐
        │ COOLDOWN   │ (Auger off, fan on for safety)
        └────┬───────┘
             │ Temp safe
             ▼
        ┌──────────┐
        │ STOPPED  │
        └────┬─────┘
             │
             ▼
        ┌─────────┐
        │  IDLE   │
        └─────────┘

Error Handling:
Any state → ERROR (on sensor error or temp fault)
ERROR state → requires manual restart
```

### Hysteresis Control

During RUNNING state, the auger is controlled using a hysteresis band around the setpoint:

- **Lower Bound** = Setpoint - (Hysteresis Band / 2)
- **Upper Bound** = Setpoint + (Hysteresis Band / 2)

**Example with 10°F band and 225°F setpoint:**
- Lower = 220°F
- Upper = 230°F

**Behavior:**
- Temp < 220°F → Auger ON (if fan is running)
- Temp between 220-230°F → Auger maintains current state
- Temp > 230°F → Auger OFF

## Project Structure

```
ESP32Smoker/
├── src/
│   ├── main.cpp                # Application entry point
│   ├── max31865.cpp            # RTD sensor driver
│   ├── relay_control.cpp       # Relay control implementation
│   ├── temperature_control.cpp # Control loop and state machine
│   ├── web_server.cpp          # REST API and web server
│   └── mqtt_client.cpp         # MQTT integration
│
├── include/
│   ├── config.h                # Configuration and pin definitions
│   ├── max31865.h              # RTD sensor header
│   ├── relay_control.h         # Relay control header
│   ├── temperature_control.h   # Control logic header
│   ├── web_server.h            # Web server header
│   └── mqtt_client.h           # MQTT client header
│
├── data/www/
│   ├── index.html              # Web interface
│   ├── style.css               # Styling
│   └── script.js               # Frontend logic
│
├── hardware/
│   ├── ESP32_PINOUT.md         # GPIO assignments
│   └── WIRING_DIAGRAM.md       # Hardware connections
│
├── docs/
│   ├── ARCHITECTURE.md         # System architecture
│   ├── API.md                  # API documentation
│   ├── HOME_ASSISTANT.md       # HA setup guide
│   └── TROUBLESHOOTING.md      # Common issues
│
├── platformio.ini              # PlatformIO configuration
├── README.md                   # This file
└── .gitignore                  # Git ignore rules
```

## Troubleshooting

### Can't Connect to WiFi
1. Check WiFi credentials in `config.h` or use AP mode
2. Verify ESP32 antenna connection
3. Check WiFi network compatibility (2.4 GHz required)

### Temperature Reading Errors
1. Verify MAX31865 SPI connections
2. Check RTD probe continuity
3. Verify correct wire mode (3-wire, 2-wire, or 4-wire)
4. Check MAX31865 address in config

### Relays Not Responding
1. Verify GPIO pin assignments
2. Check relay module power supply
3. Test relay module with direct power
4. Verify optoisolation if applicable

### MQTT Connection Issues
1. Verify broker IP and port
2. Check network connectivity
3. Verify MQTT client ID uniqueness
4. Check broker logs for connection errors

## Safety Considerations

- **Thermal Limits**: System shuts down if temp exceeds 500°F
- **Auger Interlock**: Auger cannot run without fan
- **Sensor Errors**: 3 consecutive errors trigger emergency stop
- **Startup Timeout**: 3-minute maximum startup time
- **Manual Shutdown**: Physical power removal is safest shutdown

## Development

### Adding New Features

1. **New Control Algorithm**: Modify `temperature_control.cpp`
2. **New Hardware**: Create driver in `src/`, add to `main.cpp`
3. **New API Endpoint**: Add to `web_server.cpp` route setup
4. **New Web UI Element**: Update HTML/CSS/JS in `data/www/`

### Building for Production

1. Disable `ENABLE_SERIAL_DEBUG` in `config.h`
2. Set static IP or configure DHCP
3. Use production MQTT broker
4. Customize AP credentials
5. Test all emergency scenarios
6. Implement over-the-air (OTA) updates

## License

[Specify your license here]

## Support

For issues, questions, or contributions, please refer to the project repository or documentation.

---

**Version**: 1.0.0  
**Last Updated**: January 28, 2026  
**Target Platform**: ESP32 (Arduino Framework)
