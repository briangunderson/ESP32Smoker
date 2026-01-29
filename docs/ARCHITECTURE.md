# Architecture Overview

## System Design

The ESP32 Smoker Controller is built on a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                   User Interfaces                            │
├────────────────┬──────────────────┬──────────────┬─────────┤
│  TM1638 Display │   Web Browser   │ Home Assistant│ Mobile  │
│  & Buttons      │  (HTTP/WebSocket)│   (MQTT)     │(Future) │
└────────────────┴──────────────────┴──────────────┴─────────┘
                          │
                          │ Network
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 Firmware                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │            Application Logic (main.cpp)              │   │
│  │  - Initialization                                    │   │
│  │  - Event loop & timers                               │   │
│  │  - System coordination                               │   │
│  └──────────────────────────────────────────────────────┘   │
│                          │                                    │
│         ┌────────────────┼────────────────┐                  │
│         ▼                ▼                ▼                  │
│  ┌────────────────┐ ┌─────────────┐ ┌──────────────┐      │
│  │  Temperature   │ │    Web      │ │    MQTT      │      │
│  │  Controller    │ │   Server    │ │   Client     │      │
│  │ (State Machine)│ │  (REST API) │ │(Home Asst)   │      │
│  └────────────────┘ └─────────────┘ └──────────────┘      │
│         │                                                    │
│  ┌──────┴──────────────────────────────────────────┐       │
│  │      Hardware Abstraction Layer                 │       │
│  ├─────────────────────────────────────────────────┤       │
│  │  ┌──────────┐ ┌──────────┐ ┌────────┐ ┌──────┐│       │
│  │  │   RTD    │ │  Relays  │ │TM1638  │ │LittleFS│       │
│  │  │(MAX31865)│ │ (GPIO)   │ │Display │ │(Config)│       │
│  │  └──────────┘ └──────────┘ └────────┘ └──────┘│       │
│  └─────────────────────────────────────────────────┘       │
│                          │                                    │
│         ┌────────────────┼────────────────┐                  │
│         ▼                ▼                ▼                  │
└─────────────────────────────────────────────────────────────┘
         │                ▼                ▼
         │          ┌──────────────┐ ┌──────────────┐
         │          │   SPI Bus    │ │   GPIO Pins  │
         │          │  (CLK,MOSI,  │ │  (Auger, Fan,│
         │          │   MISO, CS)  │ │   Igniter)   │
         │          └──────────────┘ └──────────────┘
         │
         └──────────────────────────┐
                                    ▼
                         ┌──────────────────┐
                         │  Physical Hardware   │
                         ├──────────────────┤
                         │ MAX31865 + PT1000│
                         │ Relay Module     │
                         │ TM1638 Display   │
                         │ WiFi Antenna     │
                         └──────────────────┘
```

## Module Descriptions

### 1. **Temperature Controller** (`temperature_control.*`)
The core control loop managing smoking sessions.

**Responsibilities:**
- State machine for startup, running, cooldown, shutdown
- Hysteresis-based temperature control
- Safety monitoring (temp limits, sensor errors)
- Relay management for auger, fan, igniter

**State Flow:**
```
IDLE → STARTUP → RUNNING → COOLDOWN → SHUTDOWN → IDLE
  ▲                 │                                 │
  │                 ▼                                 │
  └─────────── ERROR ◄────────────────────────────────┘
```

### 2. **MAX31865 RTD Driver** (`max31865.*`)
Low-level SPI communication with the temperature sensor.

**Responsibilities:**
- SPI protocol implementation
- Raw RTD value reading
- Resistance to temperature conversion (Callendar-Van Dusen)
- Fault detection

**Features:**
- Supports 2-wire, 3-wire, 4-wire configurations
- Automatic conversion mode
- Fault status monitoring

### 3. **Relay Control** (`relay_control.*`)
GPIO control for the three relays with safety interlocks.

**Responsibilities:**
- Set individual relay states (ON/OFF)
- Safety interlock: prevent auger without fan
- Emergency stop functionality
- State tracking

**Relays:**
- **Relay 1**: Pellet auger motor (12V DC)
- **Relay 2**: Combustion fan (120V AC)
- **Relay 3**: Hot rod igniter (240V AC)

### 4. **Web Server** (`web_server.*`)
HTTP server and REST API for the web dashboard.

**Endpoints:**
- `GET /api/status` - Current system status
- `POST /api/start` - Start smoking
- `POST /api/stop` - Stop/cooldown
- `POST /api/shutdown` - Emergency shutdown
- `POST /api/setpoint` - Update target temperature

**Static Files:**
- `/index.html` - Web UI
- `/style.css` - Styling
- `/script.js` - Client-side logic

### 5. **MQTT Client** (`mqtt_client.*`)
Network communication for Home Assistant integration.

**Topics Published:**
- `home/smoker/sensor/temperature` - Current temp
- `home/smoker/sensor/setpoint` - Target temp
- `home/smoker/sensor/state` - Current state
- `home/smoker/sensor/auger|fan|igniter` - Relay status

**Topics Subscribed:**
- `home/smoker/command/start` - Start session
- `home/smoker/command/stop` - Stop session
- `home/smoker/command/setpoint` - Update target

### 6. **TM1638 Display** (`tm1638_display.*`)
Physical user interface with dual 7-segment displays, LEDs, and buttons.

**Responsibilities:**
- Display current temperature and setpoint
- Status LEDs for system states (auger, fan, igniter, WiFi, MQTT, error, running)
- Button input handling with debouncing
- Standalone control without network connectivity

**Button Functions:**
- Button 1: **Start** - Begin smoking
- Button 2: **Stop** - Stop and cooldown
- Button 3: **Temp Up** - Increase setpoint by 5°F
- Button 4: **Temp Down** - Decrease setpoint by 5°F
- Button 5: **Mode** - Reserved for future use

### 7. **Configuration** (`config.h`)
Centralized configuration and pin definitions.

**Configurable Items:**
- GPIO pin assignments
- Temperature control parameters
- WiFi credentials
- MQTT settings
- Safety limits
- Debug options

## Control Algorithm

### Hysteresis Temperature Control

The system uses a hysteresis band around the target temperature to prevent relay oscillation:

```
Temperature
   ▲
   │     Upper Bound (Setpoint + Band/2)
   │        ─────────────────────────────
   │       │
   │  Auger OFF
   │       │
   │        ─────────────────────────────
   │     Setpoint (225°F)
   │        ─────────────────────────────
   │       │
   │  Auger ON (if fan on)
   │       │
   │        ─────────────────────────────
   │     Lower Bound (Setpoint - Band/2)
   │        ─────────────────────────────
   │
   └─────────────────────────────────► Time
```

**Example (225°F setpoint, 10°F band):**
- **Upper Bound**: 230°F
- **Lower Bound**: 220°F
- If T > 230°F: Auger OFF
- If T < 220°F: Auger ON
- If 220°F ≤ T ≤ 230°F: Hold current state

**Advantages:**
- Prevents oscillation
- Reduces relay wear
- Stable operation
- Simple tuning

### Startup Sequence

```
T=0s        T=60s       T=65s       T=180s
│           │           │           │
├─ Igniter ON
│   Fan OFF
│   Auger OFF
│           │
│           ├─ Igniter ON
│           │   Fan ON
│           │   Auger OFF
│           │
│           │           │
│           │           ├─ Igniter ON
│           │           │   Fan ON
│           │           │   Auger ON
│           │           │   (heating phase)
│           │           │
│           │           │           │
│           │           │           └─ TIMEOUT ERROR
│           │           │               (if not at setpoint)
│           │           │
│           │           └─ RUNNING STATE
│           │               (if setpoint reached)
│
└─────────────────────────────────┘
         STARTUP PHASE (0-180s max)
```

### Cooldown & Shutdown

```
RUNNING STATE
    │
    ├─ stop() called
    │
    ▼
COOLDOWN STATE
    ├─ Auger OFF (no more pellets)
    ├─ Fan ON (safety - cool the unit)
    ├─ Igniter OFF
    │
    └─ Wait until temp drops or timeout
         │
         ▼
    SHUTDOWN STATE
         │
         └─ All relays OFF
            │
            ▼
          IDLE STATE
```

## Data Flow Diagrams

### Temperature Reading Flow

```
RTD Probe
    │
    ▼ (Analog resistance)
MAX31865 Sensor
    │
    ├─ SPI Read (every 100ms)
    │  (RAW RTD value)
    │
    ▼
max31865.cpp::readTemperature()
    │
    ├─ Convert to resistance
    ├─ Apply Callendar-Van Dusen
    ├─ Convert to °F
    ├─ Add offset calibration
    │
    ▼ (Current temp in °F)
Temperature Controller
    │
    ├─ Average with buffer
    └─ Compare to setpoint
         │
         ▼ (Control decision)
      Relay Control
         │
         └─ Update auger/fan/igniter
```

### Control Feedback Loop

```
┌─────────────────────────────────────────┐
│        Control Feedback Loop            │
│   (every TEMP_CONTROL_INTERVAL ms)      │
├─────────────────────────────────────────┤
│                                         │
│  1. Read RTD temperature                │
│  2. Check for sensor errors             │
│  3. Execute state machine               │
│  4. Apply control logic (hysteresis)    │
│  5. Update relay states                 │
│  6. Publish MQTT status (periodic)      │
│  7. Serve web requests (async)          │
│                                         │
└─────────────────────────────────────────┘
```

### Web/MQTT Request Handling

```
HTTP/MQTT Request
    │
    ├─ API Handler
    │   ├─ Parse parameters
    │   ├─ Validate input
    │   ├─ Call controller method
    │   └─ Return JSON response
    │
    ▼
Temperature Controller
    │
    ├─ Update state (e.g., startSmoking)
    ├─ Next control loop iteration:
    │   ├─ State machine executes new state
    │   ├─ Relays update
    │   ├─ Temperature feedback
    │   └─ Status published
    │
    ▼
Client (Web/Home Assistant)
    │
    └─ Display updated status
```

## Performance Characteristics

| Component | Typical Value | Notes |
|-----------|---------------|-------|
| Temperature Update Rate | 2 seconds | Configurable |
| RTD Measurement Accuracy | ±0.15°C | Depends on MAX31865 + RTD |
| Hysteresis Band | 10°F | Prevents oscillation |
| Web Server Latency | < 100ms | Local network |
| MQTT Publish Interval | 5 seconds | Status updates |
| Startup Time to Running | 60-120s | Including preheat |
| Startup Timeout | 180 seconds | Safety limit |
| Response Time (Relay) | < 50ms | Typical electromagnetic relay |

## Error Handling Strategy

### Sensor Errors
- **Up to 2 errors**: Logged, operation continues
- **3+ consecutive errors**: Emergency stop, ERROR state
- **Recovery**: Manual restart via startSmoking()

### Temperature Faults
- **Out of safe range** (< 50°F or > 500°F): Emergency stop
- **High/low alarms**: Relay cutoff
- **Recovery**: Manual check of equipment

### Network Issues
- **WiFi disconnected**: AP mode fallback, system continues
- **MQTT offline**: Web interface still functional, status cached
- **Auto-reconnect**: Attempted every 5 seconds

### State Machine Safety
- All paths lead to IDLE or ERROR states
- No infinite loops or deadlocks
- All transitions logged for debugging

## Configuration Priority

Settings loaded in order (later override earlier):
1. Hardcoded defaults in `config.h`
2. SPIFFS configuration file (future)
3. Web UI settings (future)
4. MQTT overrides (future)

## Future Enhancements

- [ ] Over-the-air (OTA) firmware updates
- [ ] SD card logging for historical data
- [ ] PID control algorithm option
- [ ] Multiple RTD probe support
- [ ] Mobile app (iOS/Android)
- [ ] Cloud integration
- [ ] Data analytics and trends
- [ ] Multiple smoker management
