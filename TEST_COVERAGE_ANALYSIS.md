# Test Coverage Analysis — ESP32 Smoker Controller

## Current State

**Automated test coverage: 0%**

There are no unit tests, no integration tests, no test framework, and no `[env:native]` build environment configured. The CI/CD pipeline (`build.yml`) only verifies compilation — it runs no tests. All validation today is manual: the 24-item checklist in `CLAUDE.md`, the debug API endpoints, and the `runHardwareDiagnostic()` runtime check in the MAX31865 driver.

This analysis identifies the highest-value areas for introducing automated tests, ordered by risk and testability.

---

## 1. PID Controller Math (Critical)

**File**: `src/temperature_control.cpp:415-486` (`updatePID()`)

The PID algorithm is the core control loop. A bug here causes either a fire hazard (auger runs at 100% indefinitely) or a dead fire (auger never runs). The math is pure arithmetic on class members with no hardware dependencies, making it the highest-ROI test target.

### What to test

| Test case | Expected behavior |
|---|---|
| **Gain calculation** | `Kp = -1/60 = -0.01667`, `Ki = Kp/180 = -0.00009259`, `Kd = Kp*45 = -0.75` |
| **P term at setpoint** | When `currentTemp == setpoint`, P = 0.5 (centering offset) |
| **P term below setpoint** | When `currentTemp < setpoint`, P > 0.5 (more fuel) |
| **P term above setpoint** | When `currentTemp > setpoint`, P < 0.5 (less fuel) |
| **Integral anti-windup** | `_integral` clamped to `±abs(0.5 / Ki)` ≈ ±5398 |
| **Integral freeze during lid-open** | When `_lidOpen == true`, `_integral` does not change |
| **Derivative on measurement** | D = `Kd * (currentTemp - previousTemp) / dt` — not based on error |
| **No derivative kick** | Changing setpoint does not cause a D-term spike |
| **Output clamping** | Output stays within `[0.15, 1.0]` regardless of P+I+D sum |
| **dt guard** | If dt < 0.001s, `updatePID()` returns early (no divide-by-zero) |
| **Steady state convergence** | After N iterations at setpoint, output settles near 0.5 |
| **Large step response** | Step from 150 to 350 — verify no output runaway, integral winds up then clamps |

### Suggested approach

Test through the `TemperatureController` class with mock sensor/relay objects. Set `_currentTemp`, `_setpoint`, `_previousTemp`, and `_lastPidUpdate` directly, call `updatePID()`, and assert on `_pidOutput` and the individual terms. This requires making PID state accessible for testing (friend class, or test-only getters gated behind `#ifdef UNIT_TEST`).

---

## 2. State Machine Transitions (Critical)

**File**: `src/temperature_control.cpp:46-128` (dispatch), lines 258-413 (handlers)

The state machine governs all 7 operating modes. Incorrect transitions can leave relays in unsafe combinations.

### What to test

| Transition | Trigger | Verify |
|---|---|---|
| IDLE → STARTUP | `startSmoking(225)` | Relays all off initially, PID reset, reignite counter reset |
| STARTUP phase 1 (0–60s) | Time < `IGNITER_PREHEAT_TIME` | Igniter ON (if temp < 100), fan OFF, auger OFF |
| STARTUP phase 2 (60–65s) | Time in fan startup window | Igniter ON, fan ON, auger OFF |
| STARTUP phase 3 (65–180s) | Time past fan delay | Igniter ON, fan ON, auger ON |
| STARTUP → RUNNING | Temp reaches 115°F | State changes, NVS integral restored |
| STARTUP → ERROR | 180s timeout without reaching 115°F | `emergencyStop()` called |
| Igniter cutoff in STARTUP | Temp exceeds 100°F during any phase | Igniter turns OFF |
| RUNNING → COOLDOWN | `stop()` called | Auger/igniter off, fan stays on |
| RUNNING → REIGNITE | Temp < 140°F AND PID maxed for 120s AND not lid-open | Phase 0 starts |
| RUNNING → ERROR | Reignite attempts exhausted (3) | `emergencyStop()` |
| COOLDOWN → SHUTDOWN | Timeout or temp drops below 70°F | All relays off |
| SHUTDOWN → IDLE | Immediate | All relays off |
| Any active state → ERROR | Temp >= 550°F | `emergencyStop()` |
| RUNNING/COOLDOWN → ERROR | Temp <= 50°F | `emergencyStop()` |
| ERROR → IDLE | `resetError()` | Relays off, error count reset |
| ERROR state no-op | `resetError()` from non-ERROR state | No state change |

### Missing guard: `startSmoking()` from non-IDLE states

`startSmoking()` sets `_state = STATE_STARTUP` unconditionally — there is no check that the current state is IDLE. This means calling `startSmoking()` during COOLDOWN or ERROR skips safety procedures. A test should verify this behavior (and potentially a code fix should guard against it).

### Debug mode bypass

When `_debugMode == true`, `update()` only reads temperature and returns. Verify that the state machine does not advance and no relay changes occur.

---

## 3. Safety Interlocks (Critical)

**File**: `src/relay_control.cpp:79-89` (`setSafeAuger()`)

The auger interlock is the primary fire safety mechanism.

### What to test

| Test case | Expected behavior |
|---|---|
| `setSafeAuger(ON)` with fan OFF | Auger stays OFF (rejected) |
| `setSafeAuger(ON)` with fan ON | Auger turns ON |
| `setSafeAuger(OFF)` with fan OFF | Auger turns OFF (always allowed) |
| `emergencyStop()` | All 3 relays OFF |
| `allOff()` | All 3 relays OFF |
| `getStates()` | Reflects actual state after operations |
| Out-of-range relay ID | `setRelay(3, ON)` is a no-op |
| Active-low GPIO mapping | `RELAY_ON` → `digitalWrite(pin, LOW)` |

### Debug mode interlock bypass (potential issue)

`setManualRelay("auger", true)` at `temperature_control.cpp:838-841` calls `_relayControl->setAuger()` directly, **bypassing** `setSafeAuger()`. This means in debug mode, the auger can be turned on without the fan running. This is a design decision worth testing explicitly — either to verify it's intentional or to catch it as a bug.

---

## 4. Temperature Conversion (High)

**Files**: `src/max31865.cpp:272-290` (`rtdResistanceToTemperature()`), `src/temperature_control.cpp:530-568` (`readTemperature()`)

### What to test — Callendar-Van Dusen

| Resistance (Ω) | Expected Temp (°C) | Notes |
|---|---|---|
| 1000.0 | 0.0 | Exactly R₀ (PT1000 definition) |
| 1097.9 | ~25.0 | Room temperature |
| 1385.1 | ~100.0 | Boiling point |
| 1194.0 | ~50.0 | Common smoking reference |
| 900.0 | Negative | Below-zero branch (quadratic formula) |

### What to test — `readTemperature()`

| Scenario | Expected |
|---|---|
| Sensor returns valid °C | Converted to °F correctly |
| Sensor returns < -100 (error) | Returns `false`, `_consecutiveErrors` incremented |
| Temperature override enabled | Returns override value, resets error count |
| Sample averaging | After `TEMP_SAMPLE_COUNT` calls, output is averaged |

### Static buffer concern

`readTemperature()` uses `static` local variables for the averaging buffer (`tempBuffer`, `bufferIndex`, `bufferFull`). This state persists across calls and cannot be reset between tests without code changes. This is a testability issue that should be addressed — either by moving the buffer into the class, or by adding a reset method gated behind `#ifdef UNIT_TEST`.

---

## 5. Reignite Logic (High)

**File**: `src/temperature_control.cpp:659-748` (`handleReigniteState()`)

The 4-phase reignite sequence is a safety-critical recovery mechanism. It has never been tested automatically.

### What to test

| Phase | Duration | Relay states | Verify |
|---|---|---|---|
| 0: Fan Clear | 30s | Fan ON, auger OFF, igniter OFF | Ash cleared from firepot |
| 1: Igniter Preheat | 60s | Fan ON, auger OFF, igniter ON | Hot rod heated |
| 2: Feeding | 30s | Fan ON, auger ON (via `setSafeAuger`), igniter ON | Fresh pellets fed |
| 3: Recovery | 120s | Fan ON, auger 50% duty, igniter OFF | Wait for temp rise |

| Scenario | Expected |
|---|---|
| Recovery success (temp rises above 140°F) | Return to RUNNING, integral reset to 0, attempt counter incremented |
| Recovery failure (temp stays low after 120s) | Attempt counter incremented, retry from phase 0 |
| 3 failed attempts | ERROR state, `emergencyStop()` |
| Reignite trigger inhibited during lid-open | `!_lidOpen` guard prevents false triggers |
| PID output drops below max during detection window | `_pidMaxedSince` resets to 0, no reignite |
| Phase 3 auger 50% duty | Auger toggles within `AUGER_CYCLE_TIME` at 50% |

### Edge case: reignite counter incremented at different points

On success, `_reigniteAttempts` is incremented *before* returning to RUNNING. On failure, it's incremented *before* checking the max. Verify that the counter reaches exactly `REIGNITE_MAX_ATTEMPTS` before triggering ERROR (not off-by-one).

---

## 6. Lid-Open Detection (High)

**File**: `src/temperature_control.cpp:754-799` (`detectLidOpen()`)

### What to test

| Scenario | Expected |
|---|---|
| Rapid temp drop (dT/dt < -2.0°F/s) | `_lidOpen` = true, `_lidOpenTime` set |
| Gradual temp drop (dT/dt = -1.0°F/s) | No lid detection |
| Recovery: temp stabilizes for 30s | `_lidOpen` = false |
| Recovery: temp drops again during stable window | `_lidStableTime` resets |
| Integral freeze | During lid-open, `_integral` does not change in `updatePID()` |
| Only active in RUNNING state | `detectLidOpen()` not called in other states, `_lidOpen` forced false |
| Minimum duration (5s) | Lid-open flag persists even if temp stabilizes before 5s |

### Note on derivative calculation

The derivative `dTdt` is computed using `TEMP_CONTROL_INTERVAL / 1000.0` as `dt`, not the actual elapsed time. If `update()` is called at irregular intervals (e.g., due to WiFi activity or sensor retries), the derivative may be inaccurate. A test should verify behavior with irregular call timing.

---

## 7. Setpoint Validation (Medium)

**File**: `src/temperature_control.cpp:131-139` (`startSmoking()`), lines 183-188 (`setSetpoint()`)

### What to test

| Input | `startSmoking()` | `setSetpoint()` |
|---|---|---|
| 150°F (min boundary) | Accepted, state → STARTUP | Accepted |
| 149°F (below min) | Rejected, state unchanged | Rejected |
| 350°F (max from CLAUDE.md) / 500°F (max in config) | Accepted | Accepted |
| 501°F (above max) | Rejected | Rejected |
| 0°F | Rejected | Rejected |
| NaN | Behavior undefined — needs test | Behavior undefined |
| Negative temperature | Rejected | Rejected |

### Note on max setpoint discrepancy

CLAUDE.md says `TEMP_MAX_SETPOINT` is 350°F, but the config value should be checked. The web interface also needs to enforce the same limits client-side.

---

## 8. Auger Time-Proportioning (Medium)

**File**: `src/temperature_control.cpp:488-515` (`applyPIDOutput()`)

### What to test

| PID Output | Expected On-Time (of 20s cycle) |
|---|---|
| 0.15 (minimum) | 3s on, 17s off |
| 0.50 (balanced) | 10s on, 10s off |
| 1.00 (maximum) | 20s on, 0s off |
| 0.75 | 15s on, 5s off |

| Scenario | Expected |
|---|---|
| Cycle boundary crossing | Auger state is correct when `cyclePosition` wraps |
| State change minimization | Relay only toggled when desired state differs from current |
| `setSafeAuger` used for ON | Safety interlock applies even during automatic control |
| `setAuger` used for OFF | Direct call (no interlock needed for OFF) |

---

## 9. NVS Integral Persistence (Medium)

**File**: `src/temperature_control.cpp:886-981`

### What to test

| Scenario | Expected |
|---|---|
| Save on RUNNING → COOLDOWN | Integral and setpoint written to NVS |
| Save on RUNNING → ERROR | Integral and setpoint written to NVS |
| Restore with matching setpoint (±20°F) | Saved integral restored |
| Restore with different setpoint (>20°F diff) | Integral starts at 0.0 |
| First boot (no saved data) | Integral starts at 0.0 |
| Periodic save during RUNNING | Save triggers every 300s |
| `ENABLE_PID_PERSISTENCE = false` | Save/load are no-ops |

---

## 10. MQTT Command Handling (Medium)

**File**: `src/mqtt_client.cpp:154-201` (`handleMessage()`)

The MQTT command handler IS implemented (contrary to what might be assumed from its simplicity). It parses incoming MQTT commands and routes them to the controller.

### What to test

| Command | Payload | Expected |
|---|---|---|
| `start` with valid temp | `"250"` | `startSmoking(250.0)` called |
| `start` with empty payload | `""` | `startSmoking(225.0)` (default) |
| `start` with out-of-range temp | `"999"` | `startSmoking(225.0)` (falls through to default) |
| `stop` | any | `stop()` called |
| `setpoint` with valid temp | `"275"` | `setSetpoint(275.0)` called |
| `setpoint` with empty payload | `""` | No action (length check) |
| `setpoint` with out-of-range | `"0"` | No action (range check) |
| Unknown command | `"restart"` | Silently ignored |
| Non-matching topic prefix | `"other/topic"` | Returns early |
| Payload > 63 bytes | Long string | Truncated to 63 chars |

### Subtle behavior: `atof` on invalid input

`atof("abc")` returns 0.0, which is below `TEMP_MIN_SETPOINT`. This means invalid text payloads are silently rejected — but this is accidental, not validated.

---

## 11. History Ring Buffer (Low)

**File**: `src/temperature_control.cpp:903-950`

### What to test

| Scenario | Expected |
|---|---|
| First N samples (N < max) | `getHistoryCount()` = N, samples in order |
| Buffer full + wraparound | Oldest sample overwritten, `getHistorySampleAt(0)` returns oldest surviving |
| `getHistorySampleAt()` with wrapped buffer | Correct index math: `(head + index) % max` |
| Event buffer (64 max) | Same wraparound logic |
| Sample interval enforcement | Samples only recorded every `HISTORY_SAMPLE_INTERVAL` ms |
| Temperature scaling | `225.3°F` stored as `int16_t(2253)` |

---

## 12. Web API Input Validation (Low)

**File**: `src/web_server.cpp`

### What to test

| Endpoint | Bad Input | Expected |
|---|---|---|
| `POST /api/setpoint` | Missing `temp` param | 400 error |
| `POST /api/start` | `temp=999` | Rejected by controller or clamped |
| `POST /api/debug/mode` | Missing `enabled` param | 400 error |
| `POST /api/debug/relay` | Missing `relay` or `state` | 400 error |
| `POST /api/debug/relay` | Relay control without debug mode | Rejected |
| `GET /api/history` | Empty buffer | Valid JSON with empty arrays |
| `GET /api/status` | During any state | Valid JSON with all fields |

### Concurrency concern

`ESPAsyncWebServer` runs request handlers on the async TCP task, while the main loop runs the state machine. There are no mutexes protecting shared state (`_currentTemp`, `_state`, relay states, history buffer). A race condition test would be valuable — e.g., reading `/api/status` while the state machine is mid-transition.

---

## 13. Cooldown Logic (Low)

**File**: `src/temperature_control.cpp:356-374`

The cooldown-to-shutdown transition uses a compound condition that's easy to get wrong:

```cpp
if (elapsed < SHUTDOWN_COOL_TIMEOUT && _currentTemp > TEMP_MIN_SAFE + 20)
```

This means cooldown continues **only if** both the timeout hasn't elapsed AND temp is above threshold. Either condition being false triggers shutdown.

### What to test

| Scenario | Expected |
|---|---|
| Timeout expires, temp still high | Transitions to SHUTDOWN |
| Temp drops below threshold, timeout not expired | Transitions to SHUTDOWN |
| Both conditions still active | Stays in COOLDOWN, fan ON |

---

## Architectural Gaps Not Covered by Unit Tests

These are systemic issues that can't be caught by unit tests alone:

### 1. No `[env:native]` build environment

PlatformIO's `native` platform allows running C++ tests on the host machine without ESP32 hardware. Without this, there is no way to run automated tests in CI/CD.

### 2. No CI/CD test step

`.github/workflows/build.yml` only runs `pio run -e feather_esp32s3`. Even after adding tests, CI won't run them until a `pio test -e native` step is added.

### 3. Hardware coupling

Most logic is tightly coupled to Arduino APIs (`millis()`, `digitalWrite()`, `SPI`, `Preferences`, `WiFi`). To test on a host machine, these need mocks or abstractions:

| Dependency | Mocking approach |
|---|---|
| `millis()` | Inject a time function or use `#ifdef UNIT_TEST` to provide a settable mock |
| `digitalWrite()` / `pinMode()` | Record calls to verify GPIO behavior |
| `SPI` | Not needed (temperature math is testable without SPI) |
| `Preferences` (NVS) | In-memory key-value store |
| `Serial.printf()` | No-op or capture to buffer |

### 4. Static local variables in `readTemperature()`

The averaging buffer uses `static` locals that persist between test cases. Either move these into the class or add a `resetSampleBuffer()` method.

---

## Recommended Implementation Strategy

### Phase 1: Infrastructure

Add a `[env:native]` section to `platformio.ini`:

```ini
[env:native]
platform = native
test_framework = unity
build_flags = -DUNIT_TEST
build_src_filter = -<main.cpp> -<web_server.cpp> -<mqtt_client.cpp> -<tm1638_display.cpp> -<encoder.cpp> -<logger.cpp> -<telnet_server.cpp> -<tui_server.cpp> -<http_ota.cpp>
```

Create mock headers in `test/mocks/`:
- `Arduino.h` — stubs for `millis()`, `digitalWrite()`, `pinMode()`, `Serial`, `delay()`
- `Preferences.h` — in-memory NVS mock
- `SPI.h` — no-op or recording stub

### Phase 2: PID tests (highest ROI)

Write tests that:
1. Construct `TemperatureController` with mock sensor + relay
2. Set internal state directly (or via setters)
3. Call `updatePID()`
4. Assert on output values and relay calls

### Phase 3: State machine tests

Drive the controller through complete cycles:
- IDLE → STARTUP → RUNNING → COOLDOWN → SHUTDOWN → IDLE
- RUNNING → REIGNITE (4 phases) → RUNNING
- RUNNING → REIGNITE (3 failures) → ERROR
- Sensor failure → ERROR → `resetError()` → IDLE

### Phase 4: Safety interlock tests

Test `RelayControl` in isolation:
- Mock `digitalWrite()` to record calls
- Verify auger interlock logic
- Verify emergency stop

### Phase 5: CI/CD integration

Add to `.github/workflows/build.yml`:

```yaml
- name: Run unit tests
  run: pio test -e native --verbose
```

---

## Summary Table

| # | Module | Risk if Untested | Testability | Priority |
|---|--------|-----------------|-------------|----------|
| 1 | PID controller math | Fire hazard / fire dies | High (pure math) | Critical |
| 2 | State machine transitions | Unsafe relay states | High (with mocks) | Critical |
| 3 | Safety interlocks | Unburned pellet buildup | High (minimal deps) | Critical |
| 4 | Temperature conversion | Incorrect control | High (pure math) | High |
| 5 | Reignite logic | Failed recovery / stuck in reignite | Medium (timing deps) | High |
| 6 | Lid-open detection | PID overshoot / false triggers | Medium (timing deps) | High |
| 7 | Setpoint validation | Out-of-range operation | High (simple logic) | Medium |
| 8 | Auger time-proportioning | Incorrect fuel delivery | Medium (timing deps) | Medium |
| 9 | NVS integral persistence | Degraded warm-start | Medium (NVS mock) | Medium |
| 10 | MQTT command handling | Incorrect remote control | Medium (string parsing) | Medium |
| 11 | History ring buffer | Corrupt web graph data | High (pure logic) | Low |
| 12 | Web API validation | Bad input accepted | Low (async server) | Low |
| 13 | Cooldown logic | Premature/delayed shutdown | High (simple logic) | Low |
