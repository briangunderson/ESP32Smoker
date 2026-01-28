# API Reference

## Base URL

```
http://<ESP32_IP>/api
```

Default IP: `http://192.168.4.1/api` (AP mode)

## Authentication

Currently, no authentication required. For production, implement basic HTTP auth or API keys.

## Response Format

All responses are JSON.

### Success Response
```json
{
  "ok": true,
  "data": { ... }
}
```

### Error Response
```json
{
  "ok": false,
  "error": "Error message describing what went wrong"
}
```

## Endpoints

### GET /api/status

Get current system status.

**Response:**
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

**Response Fields:**
| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `temp` | float | °F | Current temperature |
| `setpoint` | float | °F | Target temperature |
| `state` | string | - | Current state (Idle, Starting, Running, Cooling Down, Shutdown, Error) |
| `auger` | boolean | - | Auger relay state (true = ON) |
| `fan` | boolean | - | Fan relay state (true = ON) |
| `igniter` | boolean | - | Igniter relay state (true = ON) |
| `runtime` | integer | ms | Time in current state |
| `errors` | integer | - | Consecutive sensor errors |

**Example cURL:**
```bash
curl http://192.168.4.1/api/status
```

---

### POST /api/start

Start a smoking session at specified temperature.

**Parameters (JSON body):**
```json
{
  "temp": 225.0
}
```

| Parameter | Type | Default | Valid Range |
|-----------|------|---------|-------------|
| `temp` | float | 225 | 150 - 350 |

**Response:**
```json
{
  "ok": true
}
```

**Errors:**
- Temperature outside valid range: Returns error
- Already in RUNNING state: Returns error

**Example cURL:**
```bash
curl -X POST http://192.168.4.1/api/start \
  -H "Content-Type: application/json" \
  -d '{"temp": 250}'
```

**State Transition:**
- Before: `Idle` or `Shutdown`
- After: `Starting` (pre-heat phase)
- Then: `Running` (once setpoint reached)

---

### POST /api/stop

Stop feeding pellets and initiate cooldown phase.

**Parameters:** None

**Response:**
```json
{
  "ok": true
}
```

**Errors:**
- Not in RUNNING state: No error, safe to call anytime

**Example cURL:**
```bash
curl -X POST http://192.168.4.1/api/stop
```

**Relay Behavior:**
- Auger: OFF (stops feeding)
- Fan: ON (safety cooling)
- Igniter: OFF

**State Transition:**
- Before: `Running`
- After: `Cooling Down`
- Then: `Shutdown` (when cool)

---

### POST /api/shutdown

Immediate shutdown of all systems.

**Parameters:** None

**Response:**
```json
{
  "ok": true
}
```

**Errors:** None

**Example cURL:**
```bash
curl -X POST http://192.168.4.1/api/shutdown
```

**Relay Behavior:**
- All relays: OFF immediately

**State Transition:**
- Any state → `Shutdown` → `Idle`

---

### POST /api/setpoint

Update target temperature without starting.

**Parameters (JSON body):**
```json
{
  "temp": 250.0
}
```

| Parameter | Type | Valid Range |
|-----------|------|-------------|
| `temp` | float | 150 - 350 |

**Response:**
```json
{
  "ok": true
}
```

**Errors:**
- Temperature outside valid range: Returns error

**Example cURL:**
```bash
curl -X POST http://192.168.4.1/api/setpoint \
  -H "Content-Type: application/json" \
  -d '{"temp": 275}'
```

**Effect:**
- Updates target temperature
- Control loop applies new hysteresis band
- Effective only if in `Running` state

---

## Status Codes

| Code | Meaning |
|------|---------|
| 200 | Successful request |
| 400 | Bad request (missing/invalid parameters) |
| 404 | Endpoint not found |
| 500 | Server error |

## Rate Limiting

None currently implemented. Recommended limits for production:
- `GET /api/status`: 1 request per second per client
- `POST /api/*`: 1 request per second per client

## Example Workflows

### Start Smoking Session

```bash
# 1. Start at 225°F
curl -X POST http://192.168.4.1/api/start \
  -H "Content-Type: application/json" \
  -d '{"temp": 225}'

# 2. Poll status
for i in {1..180}; do
  curl http://192.168.4.1/api/status
  sleep 1
done

# 3. Stop when ready
curl -X POST http://192.168.4.1/api/stop
```

### Monitor and Adjust

```bash
# Get current status
STATUS=$(curl http://192.168.4.1/api/status)
TEMP=$(echo $STATUS | jq '.temp')
STATE=$(echo $STATUS | jq '.state')

# Adjust temperature if needed
if (( $(echo "$TEMP < 200" | bc -l) )); then
  curl -X POST http://192.168.4.1/api/setpoint \
    -H "Content-Type: application/json" \
    -d '{"temp": 230}'
fi
```

### Graceful Shutdown

```bash
# Initiate cooldown
curl -X POST http://192.168.4.1/api/stop

# Wait and monitor
while true; do
  STATUS=$(curl http://192.168.4.1/api/status)
  STATE=$(echo $STATUS | jq -r '.state')
  
  if [ "$STATE" = "Idle" ]; then
    echo "Shutdown complete"
    break
  fi
  
  sleep 5
done
```

## Integration Examples

### Python

```python
import requests
import json
import time

API_BASE = "http://192.168.4.1/api"

def get_status():
    response = requests.get(f"{API_BASE}/status")
    return response.json()

def start_smoking(temp=225):
    response = requests.post(
        f"{API_BASE}/start",
        json={"temp": temp}
    )
    return response.json()

def stop_smoking():
    response = requests.post(f"{API_BASE}/stop")
    return response.json()

def set_setpoint(temp):
    response = requests.post(
        f"{API_BASE}/setpoint",
        json={"temp": temp}
    )
    return response.json()

# Example usage
if __name__ == "__main__":
    # Start smoking at 225°F
    print("Starting...")
    start_smoking(225)
    
    # Monitor for 5 minutes
    for i in range(300):
        status = get_status()
        print(f"Temp: {status['temp']}°F | "
              f"Setpoint: {status['setpoint']}°F | "
              f"State: {status['state']}")
        time.sleep(1)
    
    # Stop
    print("Stopping...")
    stop_smoking()
```

### JavaScript/Node.js

```javascript
const API_BASE = 'http://192.168.4.1/api';

async function getStatus() {
  const response = await fetch(`${API_BASE}/status`);
  return response.json();
}

async function startSmoking(temp = 225) {
  const response = await fetch(`${API_BASE}/start`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ temp })
  });
  return response.json();
}

async function stopSmoking() {
  const response = await fetch(`${API_BASE}/stop`, {
    method: 'POST'
  });
  return response.json();
}

async function setSetpoint(temp) {
  const response = await fetch(`${API_BASE}/setpoint`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ temp })
  });
  return response.json();
}

// Example: Monitor temperature
setInterval(async () => {
  const status = await getStatus();
  console.log(`${status.temp}°F (Target: ${status.setpoint}°F)`);
}, 2000);
```

### Home Assistant MQTT Automation

See [HOME_ASSISTANT.md](HOME_ASSISTANT.md) for MQTT-based integration.

## Best Practices

1. **Check Status Before Commands**
   - Always call `/api/status` first to verify current state
   - Prevents sending conflicting commands

2. **Implement Timeouts**
   - Don't wait indefinitely for state transitions
   - Add maximum wait time of 5 minutes for startup

3. **Handle Errors Gracefully**
   - Check `ok` field in response
   - Implement retry logic for network issues
   - Log error messages for debugging

4. **Rate Limit Requests**
   - Status: 1 req/sec maximum
   - Commands: 1 req/sec maximum
   - Prevents overwhelming the ESP32

5. **Use Connection Pooling**
   - Reuse HTTP connections when possible
   - Reduces latency and load

## Debugging

### Enable Verbose Logging

Set `ENABLE_SERIAL_DEBUG = true` in `config.h` and monitor serial output:

```bash
pio device monitor --baud 115200
```

### Test Endpoints

```bash
# Check if server is responding
curl -v http://192.168.4.1/api/status

# Monitor status changes
watch 'curl http://192.168.4.1/api/status | jq'

# Send command with verbose output
curl -v -X POST http://192.168.4.1/api/start \
  -H "Content-Type: application/json" \
  -d '{"temp": 225}'
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Connection refused | Server not running | Check ESP32 power and serial output |
| No response | Network issue | Verify IP, WiFi connection |
| Invalid JSON | Parse error | Check curl syntax, Content-Type header |
| Temperature stuck | Sensor error | Check MAX31865 wiring, restart |

---

**API Version**: 1.0.0  
**Last Updated**: January 28, 2026
