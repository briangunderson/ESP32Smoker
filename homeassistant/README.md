# GunderGrill - Home Assistant Integration

Complete Home Assistant integration for the GunderGrill ESP32 pellet smoker controller, featuring MQTT auto-discovery, a polished Lovelace dashboard, and comprehensive automations.

## Features

- **MQTT Auto-Discovery** — All entities auto-register in Home Assistant (no manual YAML sensor config)
- **Real-time monitoring** — Temperature, setpoint, relay states, PID performance
- **Full control** — Start/stop, setpoint adjustment, quick-start presets
- **Animated hardware indicators** — Spinning fan, pulsing igniter, rotating auger
- **Temperature history charts** — 4-hour and 24-hour interactive graphs
- **PID controller analysis** — Real-time P/I/D term visualization
- **Automated notifications** — State changes, target reached, errors, high temp, offline
- **Three dashboard views** — Main control, session history, diagnostics
- **Embedded Web UI** — ESP32 native interface available as iframe

## Prerequisites

1. **Home Assistant** with [MQTT integration](https://www.home-assistant.io/integrations/mqtt/) configured
2. **MQTT broker** (Mosquitto or similar) accessible to both HA and the ESP32
3. **ESP32 firmware** v1.2.0+ (includes MQTT Discovery support)

## Required HACS Plugins

Install these from [HACS](https://hacs.xyz/) → Frontend:

| Plugin | Purpose |
|--------|---------|
| [Mushroom](https://github.com/piitaya/lovelace-mushroom) | Modern entity cards with animations |
| [ApexCharts Card](https://github.com/RomRider/apexcharts-card) | Interactive temperature & PID charts |
| [Mini Graph Card](https://github.com/kalkih/mini-graph-card) | Compact sparkline graphs |
| [Button Card](https://github.com/custom-cards/button-card) | Custom start/stop/preset buttons |
| [card-mod](https://github.com/thomasloven/lovelace-card-mod) | Dynamic CSS styling (state-based colors) |
| [Stack In Card](https://github.com/custom-cards/stack-in-card) | Clean card grouping |

### Quick HACS Install

In Home Assistant, go to **HACS → Frontend → Explore & Download Repositories** and search for each plugin above.

## Installation

### Step 1: Package

Copy the package file to your Home Assistant config:

```bash
cp packages/gundergrill.yaml /path/to/homeassistant/config/packages/
```

Ensure packages are enabled in `configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

### Step 2: Dashboard

**Option A — YAML Dashboard (recommended):**
1. Go to **Settings → Dashboards → Add Dashboard**
2. Choose a name (e.g., "GunderGrill") and select YAML mode
3. Set the YAML path to `dashboards/gundergrill.yaml` (copy it to your HA config directory)

**Option B — Paste into existing dashboard:**
1. Open any dashboard → click the three dots → **Edit Dashboard**
2. Click three dots again → **Raw Configuration Editor**
3. Paste the contents of `dashboards/gundergrill.yaml`

### Step 3: Restart

Restart Home Assistant to load the package and new entities.

## How It Works

### MQTT Discovery

The ESP32 firmware publishes MQTT Discovery messages on boot. Home Assistant automatically creates these entities:

**Sensors:**
| Entity ID | Description |
|-----------|-------------|
| `sensor.gundergrill_temperature` | Current grill temperature (°F) |
| `sensor.gundergrill_setpoint` | Target temperature (°F) |
| `sensor.gundergrill_state` | State machine (Idle/Startup/Running/Cooldown/Stopped/Error) |
| `sensor.gundergrill_pid_output` | PID auger duty cycle (%) |
| `sensor.gundergrill_pid_proportional` | PID P term |
| `sensor.gundergrill_pid_integral` | PID I term |
| `sensor.gundergrill_pid_derivative` | PID D term |
| `sensor.gundergrill_wifi_signal` | WiFi RSSI (dBm) |
| `sensor.gundergrill_uptime` | Device uptime (seconds) |
| `sensor.gundergrill_free_memory` | ESP32 free heap (bytes) |

**Binary Sensors:**
| Entity ID | Description |
|-----------|-------------|
| `binary_sensor.gundergrill_auger` | Pellet auger motor active |
| `binary_sensor.gundergrill_fan` | Combustion fan active |
| `binary_sensor.gundergrill_igniter` | Hot rod igniter active |

**Controls:**
| Entity ID | Description |
|-----------|-------------|
| `number.gundergrill_target_temperature` | Setpoint slider (150-500°F) |
| `button.gundergrill_stop` | End Cook (cooldown) |
| `button.gundergrill_emergency_stop` | Emergency Stop (all relays off) |

**Template Sensors (from package):**
| Entity ID | Description |
|-----------|-------------|
| `sensor.gundergrill_temperature_delta` | Current temp minus setpoint |
| `sensor.gundergrill_uptime_formatted` | Human-readable uptime |
| `sensor.gundergrill_wifi_quality` | WiFi quality percentage |
| `binary_sensor.gundergrill_active` | True when smoking (Startup/Running/Cooldown) |

### Availability

The ESP32 uses MQTT Last Will & Testament (LWT) to report availability:
- **Online**: `home/smoker/status/online` = `"true"` (retained, published on connect)
- **Offline**: `home/smoker/status/online` = `"false"` (retained, published by broker on disconnect)

All entities show as "unavailable" in HA when the device is offline.

### MQTT Command Topics

| Topic | Payload | Action |
|-------|---------|--------|
| `home/smoker/command/start` | Temperature (e.g., `"225"`) | Start smoking at given temp |
| `home/smoker/command/stop` | Any | End cook, begin cooldown |
| `home/smoker/command/emergency_stop` | Any | Emergency stop (all relays off immediately) |
| `home/smoker/command/setpoint` | Temperature (e.g., `"250"`) | Update target temp |

## Dashboard Views

### View 1: Smoker (Main)

The primary control view with:
- **Status chips** — State, temperature, setpoint, WiFi at a glance
- **Temperature gauge** — Large radial gauge with gradient color
- **Setpoint slider** — Drag to adjust target temperature
- **Start/Stop buttons** — Color-coded based on state (green when available, red when active)
- **Quick-start presets** — One-tap buttons for 225°F, 250°F, 275°F
- **Hardware indicators** — Animated icons (spinning fan, rotating auger, pulsing igniter)
- **PID gauge** — Real-time auger duty cycle with needle gauge
- **PID terms chart** — 30-minute P/I/D history
- **System info** — Uptime, WiFi quality, memory, notification toggle

### View 2: Session

Designed for long cook monitoring:
- **24-hour temperature chart** — Temperature + setpoint + PID output overlaid
- **Session statistics** — Key metrics in compact vertical cards
- **WiFi signal history** — 24-hour RSSI chart

### View 3: Diagnostics

For PID tuning and troubleshooting:
- **PID analysis chart** — 2-hour detailed P/I/D term history
- **Temperature error chart** — Error band with ±5°F zone highlighted
- **Memory usage** — 24-hour free heap graph
- **Entity table** — All GunderGrill entities in one list
- **Embedded Web UI** — ESP32 native web interface as iframe

## Automations

The package includes these automations (all enabled by default):

| Automation | Trigger | Action |
|-----------|---------|--------|
| State Change Notification | Any state transition | Persistent notification |
| Target Temperature Reached | Temp within 5°F of setpoint for 30s | Persistent notification |
| Error Alert | State enters "Error" | Persistent notification |
| High Temperature Alert | Temp > 400°F for 30s | Persistent notification |
| Device Offline Alert | Device unavailable for 2 min while active | Persistent notification |
| Sync Setpoint | Setpoint changes on device | Updates `input_number` in HA |

To send mobile notifications instead of persistent notifications, replace `persistent_notification.create` with your notification service (e.g., `notify.mobile_app_your_phone`).

## Customization

### Change notification method

In the package YAML, replace:
```yaml
action: persistent_notification.create
```
with:
```yaml
action: notify.mobile_app_your_phone
```

### Add more quick-start presets

Add new scripts in the package following the pattern of `gundergrill_quick_225`.

### Adjust chart time ranges

Edit `graph_span` in the dashboard YAML (e.g., `8h`, `12h`, `48h`).

## Troubleshooting

**Entities not appearing:**
- Check that MQTT integration is configured in HA
- Verify the ESP32 is connected to the same MQTT broker
- Check HA logs for MQTT discovery messages
- Restart HA after the ESP32 has connected at least once

**Charts show no data:**
- HA needs time to collect history data — charts will populate as data comes in
- Ensure the recorder integration is enabled (it is by default)

**Dashboard cards show errors:**
- Verify all HACS plugins are installed and loaded
- Clear browser cache and reload the dashboard
- Check the browser console for JavaScript errors

**Commands not working:**
- Verify MQTT broker credentials match between ESP32 and HA
- Check ESP32 serial output for `[MQTT] Received:` messages
- Test manually: `mosquitto_pub -h broker -t home/smoker/command/stop -m stop`
