#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Trill.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

const unsigned long doubleTapWindow = 400;
const unsigned long minTapDuration = 20;
const unsigned long maxTapDuration = 300;

// --- Configuration: add/remove rows here to change the number of sensors ---
struct ChannelConfig {
  uint8_t address;
  int midiCC;
  bool doubleTapMute;
};

const ChannelConfig CHANNEL_CONFIGS[] = {
  { 0x48,  7, true  },  // sensor 1 — volume, double tap mutes
  { 0x49, 11, false },  // sensor 2 — expression
  { 0x4A, 12, false },  // sensor 3
};

const int NUM_CHANNELS = sizeof(CHANNEL_CONFIGS) / sizeof(CHANNEL_CONFIGS[0]);
// ---------------------------------------------------------------------------

struct TrillChannel {
  Trill trill;
  int midiCC;
  bool doubleTapMute;
  int lastVal = -1;
  int savedVal = 64;
  bool muted = false;
  bool wasTouching = false;
  unsigned long lastTapTime = 0;
  unsigned long touchStartTime = 0;
};

TrillChannel channels[NUM_CHANNELS];

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
      if (ch.doubleTapMute) {
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

  for (int i = 0; i < NUM_CHANNELS; i++) {
    channels[i].midiCC = CHANNEL_CONFIGS[i].midiCC;
    channels[i].doubleTapMute = CHANNEL_CONFIGS[i].doubleTapMute;
    setupChannel(channels[i], CHANNEL_CONFIGS[i].address);
    delay(2000);
    channels[i].trill.updateBaseline();
  }

  while (!USBDevice.mounted()) delay(1);
}

void loop() {
  bool anyTouch = false;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    processChannel(channels[i]);
    if (channels[i].trill.getNumTouches() > 0) anyTouch = true;
  }
  digitalWrite(13, anyTouch ? HIGH : LOW);

  delay(20);
  MIDI.read();
}