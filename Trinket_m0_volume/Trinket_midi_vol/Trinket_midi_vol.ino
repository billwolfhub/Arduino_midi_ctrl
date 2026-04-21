#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);
Adafruit_NeoPixel pixel(1, 4, NEO_GRB + NEO_KHZ800);

int lastVal = -1;
bool muted = false;
bool lastButtonState = HIGH;
const int buttonPin = 0;

void setup() {
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(buttonPin, INPUT_PULLUP);
  pixel.begin();
  pixel.setBrightness(255);  // full brightness range
  while (!USBDevice.mounted()) delay(1);
}

void loop() {
  bool buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    muted = !muted;
    if (muted) {
      MIDI.sendControlChange(7, 0, 1);
      lastVal = 0;
      pixel.setPixelColor(0, pixel.Color(255, 0, 0));
      pixel.show();
    }
  }
  lastButtonState = buttonState;

  if (!muted) {
    int raw = 0;
    for (int i = 0; i < 32; i++) {
      raw += analogRead(3);
      delay(1);
    }
    raw /= 32;

    int val = map(raw, 0, 1020, 0, 127);
    val = constrain(val, 0, 127);

    // always update pixel when not muted
    int brightness = map(val, 0, 127, 5, 255);  // min 5 so it never goes fully off
    pixel.setPixelColor(0, pixel.Color(0, brightness, 0));
    pixel.show();

    if (abs(val - lastVal) > 2) {
      MIDI.sendControlChange(7, val, 1);
      lastVal = val;
    }
  }

  delay(20);
  MIDI.read();
}