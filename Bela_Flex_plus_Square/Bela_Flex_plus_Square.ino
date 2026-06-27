#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Trill.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// ============================================================
// Timing constants for double-tap mute detection
// ============================================================
const unsigned long doubleTapWindow  = 400;
const unsigned long minTapDuration   = 20;
const unsigned long maxTapDuration   = 300;

const int AUTO_CC_START = 20;

// ============================================================
// Trill Flex sensor configuration table
// ============================================================
struct ChannelConfig {
  uint8_t address;
  int midiCC;
  bool doubleTapMute;
};

const ChannelConfig CHANNEL_CONFIGS[] = {
  { 0x48, 11, false },
  { 0x49, 64, false },
  { 0x4A, 12, false },
};

const int NUM_CONFIGURED = sizeof(CHANNEL_CONFIGS) / sizeof(CHANNEL_CONFIGS[0]);
const int MAX_CHANNELS   = 8;

// ============================================================
// Trill Square configuration
// address: I2C address (default 0x28, unchanged by jumpers)
// ccX:     CC sent for horizontal position
// ccY:     CC sent for vertical position
// ============================================================
struct SquareConfig {
  uint8_t address;
  int ccX;
  int ccY;
};

const SquareConfig SQUARE_CONFIG = { 0x28, 15, 11 };

// ============================================================
// Runtime state for Flex channels
// ============================================================
struct TrillChannel {
  Trill trill;
  int midiCC;
  bool doubleTapMute;
  int lastVal          = -1;
  int savedVal         = 64;
  bool muted           = false;
  bool wasTouching     = false;
  unsigned long lastTapTime    = 0;
  unsigned long touchStartTime = 0;
};

TrillChannel channels[MAX_CHANNELS];
int activeChannels = 0;

// ============================================================
// Runtime state for the Square sensor
// ============================================================
struct SquareChannel {
  Trill trill;
  int ccX;
  int ccY;
  int lastX = -1;
  int lastY = -1;
  bool active = false;
};

SquareChannel square;

// ============================================================
// Initialize a Trill Flex sensor
// ============================================================
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

// ============================================================
// Initialize the Trill Square sensor
// ============================================================
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

// ============================================================
// Scan for unclaimed Trill Flex sensors
// ============================================================
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

// ============================================================
// Process one Flex channel
// ============================================================
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
          MIDI.sendControlChange(ch.midiCC, 0, 1);
        } else {
          MIDI.sendControlChange(ch.midiCC, ch.savedVal, 1);
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
      MIDI.sendControlChange(ch.midiCC, val, 1);
      ch.lastVal = val;
    }
  }
}

// ============================================================
// Process the Square sensor — sends two CCs (X and Y axes)
// Raw range for Trill Square in centroid mode is 0–1792
// ============================================================
void processSquare(SquareChannel &sq) {
  sq.trill.read();
  if (sq.trill.getNumTouches() == 0) return;

  int rawX = sq.trill.touchHorizontalLocation(0);
  int rawY = sq.trill.touchLocation(0);

  int x = constrain(map(rawX, 0, 1792, 0, 127), 0, 127);
  int y = constrain(map(rawY, 0, 1792, 0, 127), 0, 127);

  if (abs(x - sq.lastX) > 1) {
    MIDI.sendControlChange(sq.ccX, x, 1);
    sq.lastX = x;
  }
  if (abs(y - sq.lastY) > 1) {
    MIDI.sendControlChange(sq.ccY, y, 1);
    sq.lastY = y;
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  TinyUSBDevice.setProductDescriptor("FlexSlider");
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(13, OUTPUT);

  Wire.begin();

  uint8_t claimedAddresses[MAX_CHANNELS] = {};
  int numClaimed = 0;

  // --- Phase 1: initialize explicitly configured Flex sensors ---
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

  // --- Phase 2: discover any remaining unclaimed Flex sensors ---
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

  // --- Phase 3: initialize Trill Square ---
  square.ccX = SQUARE_CONFIG.ccX;
  square.ccY = SQUARE_CONFIG.ccY;
  if (setupSquare(square, SQUARE_CONFIG.address)) {
    delay(2000);
    square.trill.updateBaseline();
    square.active = true;
  }

  while (!USBDevice.mounted()) delay(1);
}

// ============================================================
// LOOP
// ============================================================
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
