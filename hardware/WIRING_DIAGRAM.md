# Hardware Wiring Guide

## ESP32 Pinout

### SPI Interface (MAX31865 RTD Sensor)

```
ESP32 Pin → Signal Name → MAX31865 Pin
────────────────────────────────────────
GPIO 18   → CLK         → 6 (CLK)
GPIO 23   → MOSI        → 5 (DIN)
GPIO 19   → MISO        → 4 (DOUT)
GPIO 5    → CS (NSS)    → 3 (CS)
GND       → GND         → 2 (GND)
3.3V      → VCC         → 1 (VCC)
```

### Relay Control Pins

```
ESP32 Pin → Relay Function → Component
─────────────────────────────────────────
GPIO 12   → Auger Control     → Relay 1 IN
GPIO 13   → Fan Control       → Relay 2 IN
GPIO 14   → Igniter Control   → Relay 3 IN
GND       → Common Ground     → Relay GND
5V*       → Relay Power       → Relay VCC
```

*If using 5V relays, use a level shifter or verify 3.3V is sufficient for your relay module.

### Status LED

```
ESP32 Pin → LED
─────────────────
GPIO 2    → Status LED (+ through 330Ω resistor)
GND       → LED (-)
```

### TM1638 Display Module

```
ESP32 Pin → Signal → TM1638 Pin
──────────────────────────────────
GPIO 25   → STB    → STB (Strobe)
GPIO 26   → CLK    → CLK (Clock)
GPIO 27   → DIO    → DIO (Data I/O)
GND       → GND    → GND
5V        → VCC    → VCC
```

**Note**: TM1638 module operates at 5V logic levels but is tolerant of 3.3V ESP32 outputs. The DIO line is bidirectional and uses open-drain configuration.

## Complete Schematic

### Power Distribution

```
┌──────────────────────────────────────┐
│          Voltage Supplies             │
├──────────────────────────────────────┤
│ 5V USB (ESP32)                        │
│   ├─► ESP32 Micro-USB                │
│   └─► Relay Module VCC*               │
│                                       │
│ 3.3V (from ESP32 LDO)                │
│   ├─► MAX31865 VCC                    │
│   └─► Logic Signals (SPI, GPIO)      │
│                                       │
│ 12V or 120V AC (Relay loads)          │
│   └─► Relay Contacts                  │
│       ├─► Auger Motor (220V AC)       │
│       ├─► Fan Motor (110V AC)         │
│       └─► Igniter Element (240V AC)   │
└──────────────────────────────────────┘

* Use appropriate supply for your relay module
```

### SPI Bus Connection

```
       ┌──────────────────────┐
       │      ESP32           │
       │                      │
GPIO18 │ CLK ─────────┐       │
GPIO23 │ MOSI ────────┼──┐    │
GPIO19 │ MISO ───┐    │  │    │
GPIO 5 │ CS ─┐   │    │  │    │
       │     │   │    │  │    │
       └─────┼───┼────┼──┼────┘
             │   │    │  │
             │   │    │  │
       ┌─────┴───┴────┴──┴────────┐
       │     MAX31865 Board       │
       │                          │
       │ 1: VCC (3.3V)            │
       │ 2: GND                   │
       │ 3: CS  ◄─────────────────│
       │ 4: DOUT (MISO) ◄─────────│
       │ 5: DIN (MOSI) ◄──────────│
       │ 6: CLK ◄──────────────────│
       │ 7: RTD+ ──────┐          │
       │ 8: RTD- ──────┼──────────┤
       │ 9: RTDIN+ ────┤          │
       │10: RTDIN- ────┘          │
       │                          │
       └──────────────────────────┘
              │    │
              │    └──────────┐
              │               │
         ┌────▼───┐      ┌────▼───┐
         │  PT1000 │      │ PT1000  │
         │  Probe 1│      │ Probe 2 │
         │  (Food) │      │ (Dome)  │
         └─────────┘      └─────────┘
```

### Relay Module Connection

```
Relay Module (3-Channel)
┌────────────────────────────────────┐
│ CH1  CH2  CH3 | GND  VCC           │
├────────────────────────────────────┤
│  │    │    │    │    │             │
│  │    │    │    └─┬──┴─ 5V USB    │
│  │    │    │      │              │
│  │    │    │      └─────────┐    │
│  │    │    │                │    │
│  ├────┼────┼────┐           │    │
│  │    │    │    │      ┌────┘    │
│  │    │    │    │      │         │
└──┼────┼────┼────┼──────┼─────────┘
   │    │    │    │      │
   │    │    │    │      └─ ESP32 GND
   │    │    │    │
GPIO12  │   │    └─ GPIO 13 (Fan)
   │    │   │
GPIO14  │   └───── GPIO 12 (Auger)
   │    │
   │    └───────── GPIO 13 (Fan)
   │
   └─ GPIO 14 (Igniter)

Relay Outputs (High Voltage Side)

Relay 1 (Auger Motor):
┌─ NO  → 12V Motor + (pellet auger)
├─ COM → 12V Power Supply +
└─ NC  → (not used)

Relay 2 (Fan):
┌─ NO  → 110V AC Hot (combustion fan)
├─ COM → 110V AC Neutral
└─ NC  → (not used)

Relay 3 (Igniter):
┌─ NO  → 240V AC Hot (igniter element)
├─ COM → 240V AC Neutral
└─ NC  → (not used)
```

## RTD Probe Configuration

### 3-Wire RTD Setup (Recommended)

```
PT100 Probe
┌─────────────────┐
│  Three Wires:   │
│                 │
│ Wire A (Red)    ├─────┬──────┐
│ Wire B (Black)  ├─────┼──┐   │
│ Wire C (Green)  ├─────┤  │   │
│                 │     │  │   │
└─────────────────┘     │  │   │
                        │  │   │
                   ┌────┴──┴───┴────┐
                   │   MAX31865     │
                   │                │
                   │ RTD+ (Pin 7) ◄─┘ (Wire A)
                   │ RTD- (Pin 8) ◄──┘ (Wire B)
                   │ RTDIN+ (Pin 9) ◄─┘ (Wire C)
                   │ RTDIN- (Pin 10)   (GND)
                   │                │
                   └────────────────┘
```

### 4-Wire RTD Setup (Optional, Most Accurate)

```
PT100 Probe (4-wire variant)
┌──────────────────┐
│  Four Wires:     │
│                  │
│ Wire 1 (Red)     ├─────┬────────┐
│ Wire 2 (Black)   ├──┐  │        │
│ Wire 3 (White)   ├──┼──┼────┐   │
│ Wire 4 (Green)   ├──┘  │    │   │
│                  │     │    │   │
└──────────────────┘     │    │   │
                         │    │   │
                    ┌────┴────┴───┴──┐
                    │   MAX31865     │
                    │                │
                    │ RTD+ ◄─────────┘ (W1)
                    │ RTD- ◄──────┬────┘ (W2)
                    │ RTDIN+ ◄────┤ (W3)
                    │ RTDIN- ◄─────┴─ (W4)
                    │                │
                    └────────────────┘
```

## Temperature Probes

### Recommended Specifications
- **Type**: PT1000 RTD (1000Ω at 0°C)
- **Reference Resistor**: 4.3kΩ (for MAX31865 with PT1000)
- **Accuracy**: Class B (±0.3°C ± 0.005|t|)
- **Response Time**: < 5 seconds
- **Cable Length**: 5-10 meters typical
- **Insulation**: Stainless steel or ceramic sheath

**Note**: The system is configured for PT1000 sensors. If using PT100 (100Ω), update `MAX31865_RTD_RESISTANCE_AT_0` and `MAX31865_REFERENCE_RESISTANCE` in [config.h](../include/config.h).

### Multi-Probe Setup (Optional)

For more precise control, you can add a second probe:

```
Food Probe (Primary)
├─► MAX31865 #1 (GPIO 5 CS)
│   └─ Used for control loop setpoint

Dome/Ambient Probe (Secondary)
├─► MAX31865 #2 (GPIO 15 CS - additional board)
    └─ Used for monitoring/logging
```

This requires adding a second MAX31865 board and modifying firmware.

## Environmental Considerations

### Cable Shielding
- Keep SPI cables < 30cm or use shielded SPI cable
- Shield should be grounded at ESP32 only (not both ends)
- Keep SPI clock rate at 1MHz (default)

### Power Supply
- Use quality 5V USB with adequate current (≥ 2A)
- Add 100µF capacitor near ESP32 3.3V input
- Add 100µF capacitor near relay module VCC

### Temperature Probe
- Route probe cable away from power lines
- Use ceramic or stainless sheath for protection
- Avoid water-sensitive sealants in cable glands

## Testing Checklist

- [ ] SPI communication with MAX31865 functional
- [ ] Temperature readings within reasonable range
- [ ] All three relays toggle on/off correctly
- [ ] Status LED blinks periodically
- [ ] WiFi connects to access point
- [ ] Web interface accessible and responsive
- [ ] MQTT client connects to broker
- [ ] Control commands received and executed
- [ ] Thermal safety cutoffs functional

---

**Note**: Always verify voltages and wiring before applying power. Incorrect connections can damage the ESP32 or create electrical hazards.
