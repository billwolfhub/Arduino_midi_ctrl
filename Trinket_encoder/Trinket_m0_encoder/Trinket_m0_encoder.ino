#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

const int encA = 1;
const int encB = 2;
const int mutePin = 3;
const int ledPin = 13;

int volume = 64;
bool muted = false;
bool lastButtonState = HIGH;
int lastEncA = HIGH;
const int step = 10;

void setup() {
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(encA, INPUT_PULLUP);
  pinMode(encB, INPUT_PULLUP);
  pinMode(mutePin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  while (!USBDevice.mounted()) delay(1);
}

void loop() {
  // temporary test - LED mirrors pin 4
  digitalWrite(ledPin, digitalRead(mutePin) == LOW ? HIGH : LOW);

  int currentA = digitalRead(encA);
  if (currentA != lastEncA && currentA == LOW) {
    if (digitalRead(encB) == HIGH) {
      volume = constrain(volume + step, 0, 127);
    } else {
      volume = constrain(volume - step, 0, 127);
    }
    if (!muted) {
      MIDI.sendControlChange(7, volume, 1);
    }
  }
  lastEncA = currentA;

  bool buttonState = digitalRead(mutePin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    muted = !muted;
    if (muted) {
      MIDI.sendControlChange(7, 0, 1);
      digitalWrite(ledPin, HIGH);
    } else {
      MIDI.sendControlChange(7, volume, 1);
      digitalWrite(ledPin, LOW);
    }
  }
  lastButtonState = buttonState;

  delay(2);
  MIDI.read();
}