# Arduino Volume Controller

A USB MIDI volume controller that sends CC 7 (Master Volume) messages to control 
the master output fader in RME TotalMix FX, or any other DAW/mixer that supports 
MIDI CC control.

## Hardware Versions

### Adafruit Circuit Playground Bluefruit
- Uses the onboard light sensor as a stand-in for a potentiometer
- Left button gates MIDI transmission
- Connects via USB MIDI (TinyUSB)

### Teensy 4.0
- Uses a Spectra Symbol linear softpot ribbon sensor on pin A0
- Connects via native USB MIDI (no additional libraries needed)
- Tactile button support planned

## Hardware Required

### Bluefruit Version
- Adafruit Circuit Playground Bluefruit

### Teensy Version
- Teensy 4.0
- Spectra Symbol linear softpot ribbon sensor (10k)
- 1k pull-down resistor (recommended) — lower values reduce noise and floating
- Tactile momentary button (optional, recommended)

## Wiring (Teensy)

| Softpot Pin | Teensy Pin |
|-------------|------------|
| High end    | 3.3V       |
| Low end     | GND        |
| Wiper       | A0 (pin 14)|

- Connect a 1k resistor between A0 and GND as a pull-down

## Libraries

### Bluefruit
- Adafruit TinyUSB Library (3.7.5+)
- Adafruit Circuit Playground Library
- MIDI Library by FortySevenEffects (5.0.2+)

### Teensy
- No additional libraries needed — usbMIDI is built into Teensyduino

## Setup

### Arduino IDE
- Bluefruit: Select **Adafruit Circuit Playground Bluefruit** under Adafruit nRF52 boards
- Teensy: Select **Teensy 4.0**, set USB Type to **MIDI**

### TotalMix FX
1. Options → Enable MIDI Control
2. Options → Mixer Settings → MIDI tab
3. Set Input Port to your board's USB MIDI device
4. Uncheck **Enable Protocol Support** (disables Mackie Control)
5. Right-click the Main Out fader → Learn
6. Move the softpot — TotalMix will assign CC 7

## Known Issues
- Softpot floats to ~590 raw value when untouched with 5k pull-down resistor
- Pending fix: replace pull-down with 1k resistor and add tactile button to gate transmission

## Roadmap
- Add tactile button to gate MIDI transmission
- Replace softpot with rotary potentiometer option
- Explore BLE MIDI for wireless control
