# ESP32 Smoker Controller - Project Index

## 📖 Documentation Quick Links

### Getting Started 🚀
- **[GETTING_STARTED.md](docs/GETTING_STARTED.md)** - 5-minute quickstart guide
- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - Cheat sheet for common tasks
- **[README.md](README.md)** - Main documentation with features & usage

### Understanding the System 🏗️
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System design, data flows, control algorithms
- **[BUILD_SUMMARY.md](BUILD_SUMMARY.md)** - What was built, features, statistics

### Hardware Setup 🔌
- **[WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md)** - Pin assignments, schematics, complete wiring
- **[README.md - Hardware Requirements](README.md#hardware-requirements)** - Parts list

### Using the System 🎮
- **[API.md](docs/API.md)** - REST API endpoints with examples
- **[README.md - Usage](README.md#usage)** - Web interface walkthrough
- **[README.md - Home Assistant](README.md#home-assistant-integration)** - MQTT setup

### Development 💻
- **[ARCHITECTURE.md - Module Descriptions](docs/ARCHITECTURE.md#module-descriptions)** - Code structure
- **[README.md - Project Structure](README.md#project-structure)** - File organization
- **[include/config.h](include/config.h)** - All configuration in one place

---

## 📁 File Structure Reference

```
ESP32Smoker/
│
├── 📄 README.md                    Main documentation (START HERE)
├── 📄 BUILD_SUMMARY.md             What was built
├── 📄 QUICK_REFERENCE.md           Cheat sheet
├── 📄 platformio.ini               PlatformIO configuration
├── 📄 .gitignore                   Git ignore rules
│
├── 📂 src/                         Source code
│   ├── main.cpp                    Application entry point
│   ├── max31865.cpp                RTD sensor driver
│   ├── relay_control.cpp           Relay control
│   ├── temperature_control.cpp     Control loop & state machine
│   ├── web_server.cpp              REST API & HTTP server
│   └── mqtt_client.cpp             MQTT integration
│
├── 📂 include/                     Headers
│   ├── config.h                    Pin definitions & configuration
│   ├── max31865.h                  RTD sensor interface
│   ├── relay_control.h             Relay control interface
│   ├── temperature_control.h       Control logic interface
│   ├── web_server.h                Web server interface
│   └── mqtt_client.h               MQTT client interface
│
├── 📂 data/www/                    Web interface
│   ├── index.html                  Dashboard HTML
│   ├── style.css                   Responsive styling
│   └── script.js                   Client-side logic
│
├── 📂 hardware/                    Hardware docs
│   └── WIRING_DIAGRAM.md           Complete wiring guide
│
└── 📂 docs/                        Documentation
    ├── GETTING_STARTED.md          Quick start (5 minutes)
    ├── ARCHITECTURE.md             System design & algorithms
    └── API.md                      REST API reference
```

---

## 🎯 Common Tasks & Where to Find Info

### "I want to get this running ASAP"
1. Read: [GETTING_STARTED.md](docs/GETTING_STARTED.md) (5 min)
2. Wire: [WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md)
3. Upload: `pio run --target upload`
4. Access: http://192.168.4.1

### "I want to understand how it works"
1. Start: [README.md](README.md) - Overview
2. Deep dive: [ARCHITECTURE.md](docs/ARCHITECTURE.md) - System design
3. Code: [src/temperature_control.cpp](src/temperature_control.cpp) - Core logic

### "I want to integrate with Home Assistant"
1. Read: [README.md - Home Assistant Integration](README.md#home-assistant-integration)
2. Configure: [include/config.h](include/config.h) - MQTT settings
3. Setup: Use MQTT topics from [API.md](docs/API.md)

### "I want to control it via REST API"
1. Reference: [API.md](docs/API.md) - All endpoints
2. Examples: [API.md - Integration Examples](docs/API.md#integration-examples)
3. Test: Use curl examples in QUICK_REFERENCE.md

### "I need to customize the temperature control"
1. Settings: [include/config.h](include/config.h) - All tuning params
2. Algorithm: [src/temperature_control.cpp](src/temperature_control.cpp) - Control logic
3. Reference: [ARCHITECTURE.md - Control Algorithm](docs/ARCHITECTURE.md#control-algorithm)

### "I want to change GPIO pins"
1. Edit: [include/config.h](include/config.h) - PIN_* defines
2. Verify: [WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md) - Check your new pins
3. Rebuild: `pio run`

### "Something isn't working"
1. Check: [README.md - Troubleshooting](README.md#troubleshooting)
2. Debug: Enable `ENABLE_SERIAL_DEBUG = true` in config.h
3. Monitor: `pio device monitor --baud 115200`

---

## 📊 Project Statistics

| Metric | Value |
|--------|-------|
| **Total Files** | 20 |
| **Source Code Lines** | ~1200 |
| **Header Lines** | ~350 |
| **Documentation Lines** | ~1500 |
| **Web UI Lines** | ~950 |
| **Total Size** | ~4000 lines |
| **Core Modules** | 6 |
| **Supported Pins** | 8 |
| **API Endpoints** | 5 |
| **Documentation Pages** | 7 |

---

## ✨ Feature Summary

### Control
- ✅ Hysteresis-based temperature control with configurable band
- ✅ 4-state startup sequence (preheat → fan → auger → running)
- ✅ Graceful cooldown with safety fan operation
- ✅ Emergency shutdown on sensor error or thermal fault

### Hardware
- ✅ MAX31865 RTD sensor driver (SPI)
- ✅ 3-relay control with safety interlocks
- ✅ Auger won't run without fan (fire safety)
- ✅ Configurable GPIO pins for different boards

### Connectivity
- ✅ WiFi STA mode (configured network)
- ✅ WiFi AP mode (fallback, always available)
- ✅ REST API for programmatic control
- ✅ MQTT for Home Assistant integration

### User Interface
- ✅ Modern responsive web dashboard
- ✅ Real-time temperature and status display
- ✅ Control buttons and temperature slider
- ✅ Network and system status indicators

### Safety
- ✅ Temperature limits (50°F to 500°F)
- ✅ Auger interlock (requires fan running)
- ✅ Sensor error detection (3-strike shutdown)
- ✅ Startup timeout protection
- ✅ Emergency stop capability

---

## 🔄 Development Workflow

### Setup
```bash
# 1. Clone/open project
cd d:\localrepos\ESP32Smoker

# 2. Install dependencies (if needed)
pio pkg install
```

### Make Changes
```bash
# 1. Edit files (e.g., config.h, *.cpp)
# 2. Build locally
pio run

# 3. Fix any errors
# 4. Upload when ready
pio run --target upload
```

### Debug
```bash
# Monitor serial output
pio device monitor --baud 115200

# Or use VS Code's Serial Monitor (Ctrl+Alt+V)
```

### Deploy
```bash
# 1. Final build test
pio run

# 2. Upload to device
pio run --target upload

# 3. Verify operation
# Access http://device_ip in browser
```

---

## 📝 Configuration Map

All settings are in **[include/config.h](include/config.h)**:

| Category | Settings |
|----------|----------|
| **GPIO Pins** | 8 pins (SPI, relays, LED) |
| **Sensor** | MAX31865 config, RTD resistance values |
| **Temperature** | Min/max setpoint, hysteresis, limits |
| **Control** | Update interval, timers, timeouts |
| **WiFi** | SSID, password, AP mode settings |
| **MQTT** | Broker host/port, topics, intervals |
| **Debug** | Serial output, baud rate |

---

## 🚀 Quick Start Paths

### Path A: "Just Get It Running" (30 min)
1. Read: GETTING_STARTED.md
2. Build: `pio run && pio run --target upload`
3. Wire: Follow WIRING_DIAGRAM.md
4. Access: http://192.168.4.1

### Path B: "I Want to Understand Everything" (2 hours)
1. Read: README.md overview
2. Study: ARCHITECTURE.md deep dive
3. Explore: Source code in src/
4. Test: API endpoints from API.md
5. Integrate: Home Assistant with MQTT

### Path C: "I Want to Customize It" (1 hour)
1. Skim: README.md
2. Edit: include/config.h for your needs
3. Adjust: src/temperature_control.cpp if needed
4. Build: `pio run && pio run --target upload`
5. Test: Verify behavior changes

---

## 🔗 Important Links

### Internal Documentation
- [README.md](README.md) - Main docs
- [GETTING_STARTED.md](docs/GETTING_STARTED.md) - Quick start
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - Design docs
- [API.md](docs/API.md) - API reference
- [WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md) - Hardware setup

### External Resources
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [Arduino Framework](https://www.arduino.cc/reference/en/)
- [Home Assistant MQTT](https://www.home-assistant.io/integrations/mqtt/)

---

## 📞 Support

### Debug Serial Output
```bash
# Connect via USB and monitor
pio device monitor --baud 115200

# Look for initialization messages:
# [SETUP] Initialization complete!
# [TEMP] Temperature controller initialized
# [WIFI] Connected! IP: 192.168.X.X
```

### Common Issues
See [README.md - Troubleshooting](README.md#troubleshooting) for:
- Upload problems
- WiFi issues
- Temperature reading errors
- Relay problems
- MQTT connection issues

---

## 🎓 Learning Path

1. **New to the project?**
   - Start: [GETTING_STARTED.md](docs/GETTING_STARTED.md)
   - Time: 5 minutes

2. **Want to understand the system?**
   - Read: [README.md](README.md) + [ARCHITECTURE.md](docs/ARCHITECTURE.md)
   - Time: 30 minutes

3. **Want to use it?**
   - Follow: [WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md) + [API.md](docs/API.md)
   - Time: 1-2 hours (hardware setup)

4. **Want to modify it?**
   - Edit: [include/config.h](include/config.h)
   - Study: [src/temperature_control.cpp](src/temperature_control.cpp)
   - Time: 30 minutes - 2 hours

---

**Last Updated**: January 28, 2026  
**Project Version**: 1.0.0  
**Status**: Ready for Deployment

---

## 🎯 Next Steps

1. **Read** [GETTING_STARTED.md](docs/GETTING_STARTED.md) (5 min)
2. **Wire** hardware following [WIRING_DIAGRAM.md](hardware/WIRING_DIAGRAM.md)
3. **Build & Upload** with PlatformIO
4. **Access** web interface at http://192.168.4.1
5. **Test** all controls and features
6. **Deploy** to your smoker

**Enjoy your new smoker controller! 🔥**
