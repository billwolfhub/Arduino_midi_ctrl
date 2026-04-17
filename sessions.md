# Development Sessions

## Session 1 — Bluefruit USB MIDI Setup
**Date:** April 2026

### Goal
Get USB MIDI working on the Adafruit Circuit Playground Bluefruit (nRF52840) 
to send CC messages to RME TotalMix FX.

### Hardware
- Adafruit Circuit Playground Bluefruit (nRF52840)

### Key Challenges
- MIDIUSB library is incompatible with nRF52 architecture (AVR/SAMD only)
- Conflicting TinyUSB libraries: deleted `TinyUSB_MIDI_Device_Arduino` from 
  Documents/Arduino/libraries
- Sketch file was empty on disk despite appearing populated in Arduino IDE — 
  fixed by writing file content via Terminal using `cat >`
- No USB Stack menu option in Tools — confirmed `-DUSE_TINYUSB` is baked into 
  nRF52 board package by default, no toggle needed

### Solution
Used Adafruit TinyUSB + FortySevenEffects MIDI Library combination:
- `Adafruit_USBD_MIDI usb_midi`
- `MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI)`

### Final Sketch Behavior
- Left button gates MIDI transmission
- Light sensor maps to CC 7 velocity (0–127)
- Sends to TotalMix FX Main Out fader

### TotalMix FX Configuration
- Options → Enable MIDI Control
- Mixer Settings → MIDI tab → Input/Output: Circuit Playground Bluefruit
- Unchecked Enable Protocol Support (was interfering with plain MIDI CC)
- Right-click Main Out fader → Learn → moved light sensor to assign CC 7

---

## Session 2 — Teensy 4.0 Port
**Date:** April 2026

### Goal
Port the volume controller to a Teensy 4.0 to free up the Bluefruit for 
other projects, and replace the light sensor with a Spectra Symbol linear 
softpot ribbon sensor.

### Hardware
- Teensy 4.0
- Spectra Symbol 10k linear softpot ribbon sensor
- 5k pull-down resistor (temporary — 1k on order)

### Key Differences from Bluefruit Version
- No libraries needed — `usbMIDI` is built into Teensyduino
- USB Type must be set to **MIDI** in Arduino IDE Tools menu
- Analog range is 0–1023 (10-bit)
- Softpot wiper connects to A0 (physical pin 14)

### Wiring
- Softpot high end → 3.3V
- Softpot low end → GND
- Softpot wiper → A0 (pin 14)
- 5k pull-down between A0 and GND (1k recommended)

### Key Challenges
- 5k pull-down causes softpot to float around raw value 590-607 when untouched
- Noise level of ~15-20 raw units makes stable detection difficult
- Finger release causes snap to ~590 middle value
- Software workarounds for floating value were unreliable with 5k resistor

### Partial Software Solution
- 32-sample averaging reduces noise
- Jump detection (threshold 100) ignores large sudden value changes on release
- Full solution pending 1k resistor and tactile button arrival

### Pending
- Replace 5k pull-down with 1k resistor
- Add tactile momentary button to gate MIDI transmission
- Retune map() range once proper resistor is in place

### TotalMix FX Configuration
- Same as Bluefruit version — board appears as different USB MIDI device name
