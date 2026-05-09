#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Trill.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

const int MIDI_CC = 7; // 7 is volume.
const bool DOUBLE_TAP_MUTE = true;

Trill trill;
int lastVal = -1;
int savedVal = 64;
bool muted = false;

unsigned long lastTapTime = 0;
bool wasTouching = false;
unsigned long touchStartTime = 0;
const unsigned long doubleTapWindow = 400;
const unsigned long minTapDuration = 20;
const unsigned long maxTapDuration = 300;

void setup() {
  TinyUSBDevice.setProductDescriptor("FlexSlider");
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(13, OUTPUT);
  
  int result = trill.setup(Trill::TRILL_FLEX, 0x48);
  if (result == 0) {
    trill.setMode(Trill::CENTROID);
    delay(100);
    trill.setPrescaler(4);
    delay(200);
    trill.updateBaseline();
    delay(200);
    delay(2000);
    trill.updateBaseline();
  }
  while (!USBDevice.mounted()) delay(1);
}

void loop() {
  trill.read();
  bool touching = trill.getNumTouches() > 0;
  unsigned long now = millis();

  digitalWrite(13, touching ? HIGH : LOW);

  if (touching && !wasTouching) {
    touchStartTime = now;
  }

  if (!touching && wasTouching) {
    unsigned long touchDuration = now - touchStartTime;
    if (touchDuration >= minTapDuration && touchDuration <= maxTapDuration) {
      if (DOUBLE_TAP_MUTE) {
        if (now - lastTapTime < doubleTapWindow) {
          muted = !muted;
          if (muted) {
            savedVal = lastVal;
            MIDI.sendControlChange(MIDI_CC, 0, 1);
          } else {
            MIDI.sendControlChange(MIDI_CC, savedVal, 1);
            lastVal = savedVal;
          }
        }
        lastTapTime = now;
      }
    }
  }

  wasTouching = touching;

  if (touching && !muted) {
    int location = trill.touchLocation(0);
    int val = map(location, 0, 3712, 0, 127);
    val = constrain(val, 0, 127);

    if (abs(val - lastVal) > 1) {
      MIDI.sendControlChange(MIDI_CC, val, 1);
      lastVal = val;
    }
  }

  delay(20);
  MIDI.read();
}