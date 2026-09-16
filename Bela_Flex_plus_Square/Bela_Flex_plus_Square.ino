#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Trill.h>
#include <MIDI.h>
#include "types.h"

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

const unsigned long doubleTapWindow  = 400;
const unsigned long minTapDuration   = 20;
const unsigned long maxTapDuration   = 300;

const int AUTO_CC_START = 20;

const ChannelConfig CHANNEL_CONFIGS[] = {
  { 0x48, 21, false },
  { 0x49, 64, false },
  { 0x4A, 22, false },
};

const int NUM_CONFIGURED = sizeof(CHANNEL_CONFIGS) / sizeof(CHANNEL_CONFIGS[0]);
const int MAX_CHANNELS   = 8;

const SquareConfig SQUARE_CONFIG = { 0x28, 15, 14 };

TrillChannel channels[MAX_CHANNELS];
int activeChannels = 0;
SquareChannel square;

bool setupChannel(TrillChannel &ch, uint8_t address) {
  if (ch.trill.setup(Trill::TRILL_FLEX, address) != 0) return false;
  ch.trill.setMode(Trill::CENTROID);
  delay(100);
  ch.trill.setPrescaler(4);
  delay(200);
  ch.trill.updateBaseline();
  delay(200);
  return true;
}

bool setupSquare(SquareChannel &sq, uint8_t address) {
  if (sq.trill.setup(Trill::TRILL_SQUARE, address) != 0) return false;
  sq.trill.setMode(Trill::CENTROID);
  delay(100);
  sq.trill.setPrescaler(4);
  delay(200);
  sq.trill.updateBaseline();
  delay(200);
  return true;
}

uint8_t scanForTrill(uint8_t* claimed, int numClaimed) {
  for (uint8_t addr = 0x48; addr <= 0x4F; addr++) {
    bool skip = false;
    for (int i = 0; i < numClaimed; i++) {
      if (claimed[i] == addr) { skip = true; break; }
    }
    if (skip) continue;
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) return addr;
  }
  return 0;
}

void sendCC(int cc, int value) {
  MIDI.sendControlChange(cc, value, 1);
}

void processChannel(TrillChannel &ch) {
  ch.trill.read();
  bool touching      = ch.trill.getNumTouches() > 0;
  unsigned long now  = millis();

  if (touching && !ch.wasTouching) {
    ch.touchStartTime = now;
  }

  if (!touching && ch.wasTouching) {
    unsigned long touchDuration = now - ch.touchStartTime;
    bool isTap = (touchDuration >= minTapDuration && touchDuration <= maxTapDuration);

    if (isTap && ch.doubleTapMute) {
      if (now - ch.lastTapTime < doubleTapWindow) {
        ch.muted = !ch.muted;
        if (ch.muted) {
          ch.savedVal = ch.lastVal;
          sendCC(ch.midiCC, 0);
        } else {
          sendCC(ch.midiCC, ch.savedVal);
          ch.lastVal = ch.savedVal;
        }
      }
      ch.lastTapTime = now;
    }
  }

  ch.wasTouching = touching;

  if (touching && !ch.muted) {
    int location = ch.trill.touchLocation(0);
    int val = constrain(map(location, 0, 3712, 0, 127), 0, 127);
    if (abs(val - ch.lastVal) > 1) {
      sendCC(ch.midiCC, val);
      ch.lastVal = val;
    }
  }
}

void processSquare(SquareChannel &sq) {
  sq.trill.read();
  if (sq.trill.getNumTouches() == 0) return;

  int rawX = sq.trill.touchHorizontalLocation(0);
  int rawY = sq.trill.touchLocation(0);

  int x = constrain(map(rawX, 0, 1792, 0, 127), 0, 127);
  int y = constrain(map(rawY, 0, 1792, 0, 127), 0, 127);

  if (abs(x - sq.lastX) > 1) {
    sendCC(sq.ccX, x);
    sq.lastX = x;
  }
  if (abs(y - sq.lastY) > 1) {
    sendCC(sq.ccY, y);
    sq.lastY = y;
  }
}

void setup() {
  TinyUSBDevice.setProductDescriptor("FlexSlider");
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);

  while (!USBDevice.mounted()) delay(1);

  pinMode(13, OUTPUT);
  Wire.begin();

  uint8_t claimedAddresses[MAX_CHANNELS] = {};
  int numClaimed = 0;

  for (int i = 0; i < NUM_CONFIGURED; i++) {
    uint8_t addr = CHANNEL_CONFIGS[i].address;
    claimedAddresses[numClaimed++] = addr;

    channels[activeChannels].midiCC        = CHANNEL_CONFIGS[i].midiCC;
    channels[activeChannels].doubleTapMute = CHANNEL_CONFIGS[i].doubleTapMute;

    if (!setupChannel(channels[activeChannels], addr)) {
      uint8_t found = scanForTrill(claimedAddresses, numClaimed - 1);
      if (found != 0) {
        claimedAddresses[numClaimed - 1] = found;
        if (setupChannel(channels[activeChannels], found)) {
          channels[activeChannels].midiCC        = 7;
          channels[activeChannels].doubleTapMute = false;
        } else {
          continue;
        }
      } else {
        continue;
      }
    }

    delay(2000);
    channels[activeChannels].trill.updateBaseline();
    activeChannels++;
  }

  int autoCC = AUTO_CC_START;

  for (uint8_t scan = 0x48; scan <= 0x4F && activeChannels < MAX_CHANNELS; scan++) {
    bool skip = false;
    for (int c = 0; c < numClaimed; c++) {
      if (claimedAddresses[c] == scan) { skip = true; break; }
    }
    if (skip) continue;

    Wire.beginTransmission(scan);
    if (Wire.endTransmission() != 0) continue;

    channels[activeChannels].midiCC        = autoCC++;
    channels[activeChannels].doubleTapMute = false;

    if (setupChannel(channels[activeChannels], scan)) {
      claimedAddresses[numClaimed++] = scan;
      delay(2000);
      channels[activeChannels].trill.updateBaseline();
      activeChannels++;
    }
  }

  square.ccX = SQUARE_CONFIG.ccX;
  square.ccY = SQUARE_CONFIG.ccY;
  if (setupSquare(square, SQUARE_CONFIG.address)) {
    delay(2000);
    square.trill.updateBaseline();
    square.active = true;
  }
}

void loop() {
  bool anyTouch = false;

  for (int i = 0; i < activeChannels; i++) {
    processChannel(channels[i]);
    if (channels[i].trill.getNumTouches() > 0) anyTouch = true;
  }

  if (square.active) {
    processSquare(square);
    if (square.trill.getNumTouches() > 0) anyTouch = true;
  }

  digitalWrite(13, anyTouch ? HIGH : LOW);
  delay(20);
  MIDI.read();
}