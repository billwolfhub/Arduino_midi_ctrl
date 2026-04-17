#include <Adafruit_TinyUSB.h>
#include <Adafruit_CircuitPlayground.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

void setup() {
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  CircuitPlayground.begin();
  while (!USBDevice.mounted()) delay(1);
}

void loop() {
  if (CircuitPlayground.leftButton()) {
    int light = CircuitPlayground.lightSensor();
    light = constrain(light, 0, 36);
    light = map(light, 0, 36, 0, 127);

    MIDI.sendControlChange(7, light, 1);
    delay(20);
  }
}