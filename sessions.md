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
