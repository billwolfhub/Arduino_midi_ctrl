#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Trill.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

const bool DOUBLE_TAP_MUTE = false ;
const unsigned long doubleTapWindow = 400;
const unsigned long minTapDuration = 20;
const unsigned long maxTapDuration = 300;

struct TrillChannel {
  Trill trill;
  int midiCC;
  int lastVal = -1;
  int savedVal = 64;
  bool muted = false;
  bool wasTouching = false;
  unsigned long lastTapTime = 0;
  unsigned long touchStartTime = 0;
};

TrillChannel ch1, ch2;

void setupChannel(TrillChannel &ch, uint8_t address) {
  if (ch.trill.setup(Trill::TRILL_FLEX, address) == 0) {
    ch.trill.setMode(Trill::CENTROID);
    delay(100);
    ch.trill.setPrescaler(4);
    delay(200);
    ch.trill.updateBaseline();
    delay(200);
  }
}

void processChannel(TrillChannel &ch) {
  ch.trill.read();
  bool touching = ch.trill.getNumTouches() > 0;
  unsigned long now = millis();

  if (touching && !ch.wasTouching) {
    ch.touchStartTime = now;
  }

  if (!touching && ch.wasTouching) {
    unsigned long touchDuration = now - ch.touchStartTime;
    if (touchDuration >= minTapDuration && touchDuration <= maxTapDuration) {
      if (DOUBLE_TAP_MUTE) {
        if (now - ch.lastTapTime < doubleTapWindow) {
          ch.muted = !ch.muted;
          if (ch.muted) {
            ch.savedVal = ch.lastVal;
            MIDI.sendControlChange(ch.midiCC, 0, 1);
          } else {
            MIDI.sendControlChange(ch.midiCC, ch.savedVal, 1);
            ch.lastVal = ch.savedVal;
          }
        }
        ch.lastTapTime = now;
      }
    }
  }

  ch.wasTouching = touching;

  if (touching && !ch.muted) {
    int location = ch.trill.touchLocation(0);
    int val = map(location, 0, 3712, 0, 127);
    val = constrain(val, 0, 127);

    if (abs(val - ch.lastVal) > 1) {
      MIDI.sendControlChange(ch.midiCC, val, 1);
      ch.lastVal = val;
    }
  }
}

void setup() {
  TinyUSBDevice.setProductDescriptor("FlexSlider");
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(13, OUTPUT);

  ch1.midiCC = 12;   // volume is 7
  ch2.midiCC = 11;  // expression is 11 — change to whatever you need

  setupChannel(ch1, 0x48);
  delay(2000);
  ch1.trill.updateBaseline();

  setupChannel(ch2, 0x49);
  delay(2000);
  ch2.trill.updateBaseline();

  while (!USBDevice.mounted()) delay(1);
}

void loop() {
  processChannel(ch1);
  processChannel(ch2);

  bool anyTouch = ch1.trill.getNumTouches() > 0 || ch2.trill.getNumTouches() > 0;
  digitalWrite(13, anyTouch ? HIGH : LOW);

  delay(20);
  MIDI.read();
}
