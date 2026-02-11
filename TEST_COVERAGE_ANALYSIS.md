# Test Coverage Analysis — ESP32 Smoker Controller

## Current State

**Automated test coverage: 0%**

The project has no unit tests, no integration tests, no test framework configured, and no CI/CD pipeline. All validation is manual — either through the 16-item testing checklist in `CLAUDE.md`, the debug API endpoints, or the `runHardwareDiagnostic()` function in the MAX31865 driver.

This analysis identifies the highest-value areas for introducing automated tests, ordered by risk and complexity.

---

## 1. PID Controller Logic (Critical Priority)

**File**: `src/temperature_control.cpp:336-398` (`updatePID()`)

The PID algorithm is the core control loop — the reason this project exists. It is pure math with no hardware dependencies once you abstract away the sensor and relay objects, making it highly testable.

### What to test

- **Gain calculation**: Verify `_Kp`, `_Ki`, `_Kd` are correctly derived from Proportional Band parameters
  - `Kp = -1/PB = -1/60 = -0.01667`
  - `Ki = Kp/Ti = -0.01667/180 = -0.00009259`
  - `Kd = Kp*Td = -0.01667*45 = -0.75`
- **Proportional term with 0.5 centering**: At setpoint, `P` should equal `0.5`; below setpoint `P > 0.5`; above setpoint `P < 0.5`
- **Integral anti-windup**: `_integral` must be clamped to `±abs(0.5 / _Ki)` ≈ ±5398. Verify clamping works in both directions
- **Derivative on measurement**: Verify derivative is computed from `(_currentTemp - _previousTemp) / dt`, not from error changes (prevents derivative kick on setpoint changes)
- **Output clamping**: Output must stay within `[PID_OUTPUT_MIN (0.15), PID_OUTPUT_MAX (1.0)]`
- **Edge cases**: `dt < 0.001` guard (avoids divide-by-zero), steady-state behavior, large step changes

### Why this matters

A bug in the PID output could cause the auger to run at 100% indefinitely (fire hazard) or never run (fire dies). The math is straightforward to validate in isolation.

### Suggested approach

Extract PID computation into a testable function or test through the `TemperatureController` class with mocked sensor/relay objects. Use PlatformIO's `native` environment with a test framework (Unity or GoogleTest) so tests run on the host machine without ESP32 hardware.

---

## 2. State Machine Transitions (Critical Priority)

**File**: `src/temperature_control.cpp:80-99` (state dispatch), lines 217-334 (state handlers)

The state machine governs all operating modes: `IDLE → STARTUP → RUNNING → COOLDOWN → SHUTDOWN → IDLE`, plus `ERROR` from any state.

### What to test

- **Valid transitions**: `startSmoking()` from IDLE → STARTUP, reaching threshold → RUNNING, `stop()` → COOLDOWN → SHUTDOWN → IDLE
- **Startup phases**: Igniter-only for first 60s, fan starts at 60s, auger starts at 65s, transition to RUNNING at 115°F
- **Startup timeout**: If temperature doesn't reach 115°F within 180s, state goes to ERROR and `emergencyStop()` is called
- **Igniter cutoff**: Igniter turns off when temp exceeds 100°F during startup (regardless of phase)
- **Cooldown behavior**: Auger off, igniter off, fan stays on until timeout or temp drops below `TEMP_MIN_SAFE + 20`
- **Error entry from temperature fault**: Temp below 50°F or above 550°F → ERROR
- **Error entry from sensor failure**: 5 consecutive read failures → ERROR + emergency stop
- **Error recovery**: `resetError()` only works from ERROR state, returns to IDLE
- **Integral persistence**: Integral is saved to NVS when leaving RUNNING, restored when re-entering RUNNING (if setpoint within ±20°F)
- **Debug mode bypass**: `update()` skips state machine when `_debugMode == true`

### Why this matters

Incorrect state transitions can leave relays in unsafe combinations (e.g., auger running without fan, igniter running indefinitely).

---

## 3. Safety Interlocks (Critical Priority)

**File**: `src/relay_control.cpp:79-89` (`setSafeAuger()`)

### What to test

- **Auger interlock**: `setSafeAuger(RELAY_ON)` must be rejected when fan is OFF
- **Auger interlock**: `setSafeAuger(RELAY_ON)` must succeed when fan is ON
- **Emergency stop**: `emergencyStop()` turns all relays OFF
- **allOff()**: All three relays (auger, fan, igniter) are set to OFF
- **Relay state tracking**: `getStates()` returns correct booleans after set operations
- **Out-of-range relay ID**: `setRelay()` with `relay >= RELAY_COUNT` is a no-op

### Why this matters

The auger interlock is the primary fire safety mechanism. If unburned pellets accumulate without airflow, reignition creates a dangerous flare-up.

---

## 4. Temperature Reading & Conversion (High Priority)

**Files**: `src/temperature_control.cpp:443-481` (`readTemperature()`), `src/max31865.cpp:272-290` (`rtdResistanceToTemperature()`)

### What to test

- **Callendar-Van Dusen equation**: Verify resistance-to-temperature conversion against known PT1000 values:
  - 1000.0Ω → 0.0°C
  - 1097.9Ω → 25.0°C
  - 1385.1Ω → 100.0°C
  - ~1194Ω → 50.0°C (common smoking temperature reference point)
- **Negative temperature handling**: Resistance < 1000Ω uses quadratic formula branch
- **C-to-F conversion**: `readTemperature()` correctly converts with offset: `tempC * 9/5 + 32 + TEMP_SENSOR_OFFSET`
- **Sample averaging**: The rolling buffer in `readTemperature()` averages `TEMP_SAMPLE_COUNT` samples after buffer is full; uses single value before buffer fills
- **Error propagation**: Sensor returning `< -100` (error value) causes `readTemperature()` to return `false`
- **Temperature override**: When `_tempOverrideEnabled`, `readTemperature()` returns the override value and resets error count

### Why this matters

Incorrect temperature conversion leads to wrong PID behavior. The Callendar-Van Dusen equation has two branches (positive/negative temps) and is easy to get wrong.

---

## 5. Setpoint Validation (Medium Priority)

**File**: `src/temperature_control.cpp:102-128` (`startSmoking()`), line 149-154 (`setSetpoint()`)

### What to test

- **Range enforcement**: Setpoints below `TEMP_MIN_SETPOINT` (150°F) and above `TEMP_MAX_SETPOINT` (500°F) are rejected
- **startSmoking() rejects invalid temp**: Returns without changing state
- **setSetpoint() rejects invalid temp**: Returns without changing `_setpoint`
- **Boundary values**: Exactly 150°F (accepted), 149°F (rejected), 500°F (accepted), 501°F (rejected)

---

## 6. Auger Time-Proportioning (Medium Priority)

**File**: `src/temperature_control.cpp:401-428` (`applyPIDOutput()`)

### What to test

- **Duty cycle calculation**: PID output of 0.5 → auger ON for 10s out of 20s cycle
- **Minimum duty cycle**: PID output clamped at 0.15 → auger ON for 3s out of 20s
- **Maximum duty cycle**: PID output of 1.0 → auger ON for full 20s cycle
- **Cycle wrapping**: Verify auger state is correct across cycle boundaries
- **State change minimization**: Relay is only toggled when the desired state changes (reduces relay wear)

---

## 7. NVS Integral Persistence (Medium Priority)

**File**: `src/temperature_control.cpp:646-684`

### What to test

- **Save on state exit**: Integral and setpoint are saved when leaving RUNNING
- **Restore within tolerance**: Saved integral is restored when new setpoint is within ±20°F of saved setpoint
- **Discard outside tolerance**: Saved integral is discarded (set to 0) when setpoint differs by >20°F
- **First boot**: No saved data → integral starts at 0.0
- **Periodic save**: Save triggers every `PID_SAVE_INTERVAL` (300s) during RUNNING
- **Persistence disabled**: When `ENABLE_PID_PERSISTENCE` is false, save/load are no-ops

---

## 8. Web API Input Validation (Low Priority, but worth covering)

**File**: `src/web_server.cpp`

### What to test

- **Missing parameters**: POST to `/api/setpoint` without `temp` → 400 error
- **Missing parameters**: POST to `/api/debug/mode` without `enabled` → 400 error
- **Missing parameters**: POST to `/api/debug/relay` without `relay` or `state` → 400 error
- **Invalid setpoint passthrough**: Setting temp to 0°F or 9999°F — does the controller reject it?
- **Start with default temp**: POST to `/api/start` without `temp` → uses 225°F default
- **Debug relay without debug mode**: Relay control is ignored (verified in controller, not web server)

---

## 9. MQTT Message Formatting (Low Priority)

**File**: `src/mqtt_client.cpp:80-118`

### What to test

- **Topic construction**: All topics correctly prefixed with `home/smoker/`
- **Temperature formatting**: Published as "225.0" (one decimal), not "225.00000"
- **Relay state strings**: Published as "on"/"off" (lowercase), not "true"/"false"
- **State name**: Published using `getStateName()` output (e.g., "Running", not "STATE_RUNNING")
- **messageCallback()**: Currently unimplemented — incoming MQTT commands are silently dropped. This is a functional gap, not just a test gap.

---

## Recommended Implementation Strategy

### Phase 1: Set up the native test environment

Add a `[env:native]` section to `platformio.ini` for host-based testing. PlatformIO's Unity framework is the simplest option for C/C++ embedded projects. Create mock/stub implementations of `MAX31865` and `RelayControl` that record calls rather than touching hardware.

```ini
[env:native]
platform = native
test_framework = unity
build_flags = -DUNIT_TEST
```

### Phase 2: Test the PID math (highest ROI)

The PID calculation in `updatePID()` is pure arithmetic on class member variables. With mocked dependencies, you can:
1. Set `_currentTemp`, `_setpoint`, `_previousTemp`, and `_lastPidUpdate` directly
2. Call `updatePID()`
3. Assert `_pidOutput` is within expected bounds

This catches regressions in the control algorithm without any hardware.

### Phase 3: Test state transitions

With mocked sensor (returns configurable temperatures) and mocked relays (records on/off calls), you can drive the state machine through all transitions and verify:
- Correct relay states at each phase
- Correct transition triggers
- Safety mechanisms fire when they should

### Phase 4: Test safety interlocks

`RelayControl` is already nearly hardware-independent (it just calls `digitalWrite`). Mock `digitalWrite` and `pinMode` in the native environment and test the interlock logic directly.

### Key architectural consideration

Most of the business logic is tightly coupled to Arduino APIs (`millis()`, `digitalWrite()`, `SPI`, `Preferences`). To make modules testable on a host machine, you'll need to either:

1. **Inject time**: Pass a `millis()` function pointer or use a time interface that can be stubbed in tests
2. **Abstract hardware**: The existing `MAX31865*` and `RelayControl*` pointers in `TemperatureController` already provide seams — create test doubles that implement the same interface
3. **Compile-time mocks**: Use `#ifdef UNIT_TEST` to swap Arduino-specific calls with stubs

---

## Summary Table

| Priority | Module | Risk if Untested | Testability |
|----------|--------|-----------------|-------------|
| Critical | PID controller math | Fire hazard / fire dies | High (pure math) |
| Critical | State machine transitions | Unsafe relay states | High (with mocks) |
| Critical | Safety interlocks | Unburned pellet accumulation | High (minimal deps) |
| High | Temperature conversion | Incorrect control behavior | High (pure math) |
| Medium | Setpoint validation | Out-of-range operation | High (simple logic) |
| Medium | Auger time-proportioning | Incorrect fuel delivery | Medium (timing deps) |
| Medium | NVS integral persistence | Degraded warm-start | Medium (NVS mock needed) |
| Low | Web API validation | Bad input accepted | Low (async web server) |
| Low | MQTT formatting | Home Assistant confusion | Medium (string checks) |
