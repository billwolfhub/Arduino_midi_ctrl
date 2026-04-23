## Session 2 — Teensy 4.0 Port
**Date:** April 2026

### Goal
Port the volume controller to a Teensy 4.0 to free up the Bluefruit for 
other projects, and replace the light sensor with a Spectra Symbol linear 
softpot ribbon sensor.

### Hardware
- Teensy 4.0
- Spectra Symbol 10k linear softpot ribbon sensor
- 1k pull-down resistor

### Key Differences from Bluefruit Version
- No libraries needed — `usbMIDI` is built into Teensyduino
- USB Type must be set to **MIDI** in Arduino IDE Tools menu
- Analog range is 0–1023 (10-bit)
- Softpot wiper connects to A0 (physical pin 14)

### Wiring
- Softpot high end → 3.3V
- Softpot low end → GND
- Softpot wiper → A0 (pin 14)
- 1k pull-down resistor between A0 and GND

### Key Challenges
- Initial attempts used 5k pull-down causing softpot to float around 590 when untouched
- Resistor was briefly shorting against an adjacent pin causing bad readings
- 1k pull-down resolved floating — untouched value dropped to ~2
- Softpot still noisy and mechanically inconsistent — rotary pot on order

### Measured Values (with 1k pull-down)
- Untouched: ~2
- Low end: ~19
- High end: ~800

### Final Sketch
- 32-sample averaging for noise reduction
- Dead zone below 15 raw to ignore untouched state
- Jump threshold of 100 to ignore finger release snap
- `map(raw, 19, 800, 0, 127)` tuned to actual measured range
- `constrain(0, 127)` to prevent out of range values

### Pending
- Replace softpot with rotary potentiometer for cleaner stable readings
- Add tactile momentary button to gate MIDI transmission
- Retune map() range once rotary pot is connected
- Remove dead zone logic if rotary pot reads cleanly at rest

---

## Session 3 — Rotary Potentiometer
**Date:** April 2026

### Goal
Replace the Spectra Symbol softpot with a quality 10k rotary potentiometer
for cleaner, more stable readings.

### Hardware
- Teensy 4.0
- 10k rotary potentiometer
- No pull-down

---

## Session 4 — Adafruit Trinket M0
**Date:** April 2026

### Goal
Port the volume controller to an Adafruit Trinket M0 (SAMD21) as a smaller,
cheaper alternative to the Teensy 4.0, and add a toggle mute button.

### Hardware
- Adafruit Trinket M0 (SAMD21)
- 10k rotary potentiometer
- Toggle switch (wire to GND for testing)

### Key Differences from Teensy Version
- Requires TinyUSB + FortySevenEffects MIDI library stack (same as Bluefruit)
- USB Stack must be set to TinyUSB in Arduino IDE Tools menu
- No pull-down resistor needed for pot

### Wiring
| Component | Trinket M0 Pin |
|-----------|----------------|
| Pot left leg | GND |
| Pot right leg | 3.3V |
| Pot wiper | Pin 3 (A1) |
| Mute switch | Pin 0 ↔ GND |

### Known Issues
- Pin 2 (A0) appears damaged on this board — use Pin 3 (A1) instead
- TinyUSB library controls the onboard NeoPixel for USB status and cannot be easily overridden
- Red LED on pin 13 used for mute indicator instead

### Measured Values
- Fully clockwise: ~0
- Fully counterclockwise: ~1020
- Center: ~488

### Final Sketch Behavior
- Pot on pin 3 sends CC 7 on MIDI channel 1
- Toggle switch on pin 0 toggles mute (sends CC 7 value 0)
- Red LED on pin 13 ligh


---

## Session 5 — ANO Rotary Encoder
**Date:** April 2026

### Goal
Replace the rotary potentiometer with an Adafruit ANO Rotary Navigator encoder
for a more compact control surface with built-in buttons.

### Hardware
- Adafruit Trinket M0 (SAMD21)
- Adafruit ANO Rotary Navigator encoder with breakout board
- No pull-down resistors needed

### Wiring
| ANO Pin | Trinket M0 Pin |
|---------|----------------|
| ENCA | Pin 1 |
| ENCB | Pin 2 |
| COMA | GND |
| SW1 (mute button) | Pin 4 |
| COMB | GND |

### Key Differences from Pot Version
- Encoder sends relative increments rather than absolute position
- Volume tracked in software starting at 64 (midpoint)
- No calibration needed
- ANO connects directly to digital pins — no I2C/seesaw library needed
- Pin 2 has damaged analog function but works fine for digital I2C

### Final Sketch Behavior
- Encoder increments/decrements volume by 10 per detent
- Volume constrained to 0-127
- SW1 on pin 4 toggles mute
- Mute sends CC 7 value 0, unmute restores last volume
- Red LED on pin 13 lights when muted
- Encoder read without interrupts for cleaner response
- 2ms loop delay for responsiveness

### Key Challenges
- Interrupt-based encoder reading was erratic — switched to polling in loop
- Debounce tuning needed to balance responsiveness and accuracy
- SW1 wiring error initially — confirmed pin 4 works correctly
- TinyUSB still controls NeoPixel — unresolved, left as is

### Pending
- Consider adding remaining ANO buttons (SW2-SW4) for additional functions
- Mount in enclosure
- Investigate NeoPixel control conflict with TinyUSB


---

## Session 6 — ANO Rotary Encoder
**Date:** April 2026

### Goal
Replace the rotary potentiometer with an Adafruit ANO Rotary Navigator encoder
for a more compact control surface with built-in buttons.

### Hardware
- Adafruit Trinket M0 (SAMD21)
- Adafruit ANO Rotary Navigator encoder with breakout board

### Wiring
| ANO Pin | Trinket M0 Pin |
|---------|----------------|
| ENCA | Pin 1 |
| ENCB | Pin 2 |
| COMA | GND |
| SW1 (mute button) | Pin 4 |
| COMB | GND |

### Key Notes
- ANO connects directly to digital pins — no I2C or special library needed
- Encoder read by polling in loop rather than interrupts — more stable
- Volume tracked in software starting at 64 (midpoint)
- Step size of 10 per detent gives good range coverage
- Pin 2 has damaged analog function but works fine for digital
- TinyUSB still controls onboard NeoPixel — unresolved

### Final Sketch Behavior
- Encoder increments/decrements volume by 10 per detent
- Volume constrained to 0-127
- SW1 on pin 4 toggles mute
- Mute sends CC 7 value 0, unmute restores last volume
- Red LED on pin 13 lights when muted
- 2ms loop delay for responsiveness

### Pending
- Consider adding remaining ANO buttons for additional functions
- Mount in enclosure

---

---

## Session 7 — Bela Trill Flex
**Date:** April 2026

### Goal
Use a Bela Trill Flex capacitive touch sensor as a slider input for
volume control on Teensy 4.0.

### Hardware
- Bela Trill Flex Rev C2 with breakout board
- Teensy 4.0
- QWIIC/Stemma QT cable

### Wiring (Teensy 4.0)
| Trill Breakout | Teensy 4.0 Pin |
|----------------|----------------|
| SDA | Pin 18 (A4) |
| SCL | Pin 19 (A5) |
| VCC | 3.3V |
| GND | GND |

### Key Challenges
- Trinket M0 I2C did not detect the Trill at all — switched to Teensy 4.0
- All channels reading 4095 initially — ribbon cable was inserted upside down
- CENTROID mode returned no touches — fix from Bela forum required
  setPrescaler() with delays before and after, plus double updateBaseline()
  to prevent pad oversaturation

### Fix
```cpp
trill.setMode(Trill::CENTROID);
delay(100);
trill.setPrescaler(4);
delay(200);
trill.updateBaseline();
delay(200);
delay(2000);
trill.updateBaseline();
```

### Measured Values
- Low end: 0
- High end: 3712
- Center: ~1638

### Final Sketch Behavior
- Trill Flex slider sends CC 7 on MIDI channel 1
- Only sends when touch is detected (finger on sensor)
- Releases cleanly when finger lifted
- `map(location, 0, 3712, 0, 127)` tuned to actual range
- Noise threshold of 1 to prevent jitter
- Very smooth and responsive

### Libraries
- Trill by Bela
- usbMIDI built into Teensyduino

### Pending
- Add mute button
- Mount in enclosure
- Consider adding LED indicator

### Trinket M0 Compatibility Test
- Attempted to use Trill Flex with Trinket M0 after success with Teensy 4.0
- I2C scanner found no devices at any address
- Confirmed Trinket M0 I2C is incompatible with Trill Flex
- Trinket M0 I2C works for other devices but not the Trill Flex
- Trill Flex requires Teensy 4.0 or similar board with robust I2C implementation
- Trinket M0 remains the preferred board for pot and encoder versions