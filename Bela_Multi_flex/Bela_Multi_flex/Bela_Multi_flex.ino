#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <Trill.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// ============================================================
// Timing constants for double-tap mute detection
// ============================================================
const unsigned long doubleTapWindow  = 400;  // ms between taps to count as double-tap
const unsigned long minTapDuration   = 20;   // ms — ignore accidental grazes shorter than this
const unsigned long maxTapDuration   = 300;  // ms — ignore holds longer than this

// ============================================================
// CC assigned to the first auto-discovered sensor.
// Additional discovered sensors get AUTO_CC_START+1, +2, etc.
// These are temporary until sensors are added to CHANNEL_CONFIGS.
// ============================================================
const int AUTO_CC_START = 20;

// ============================================================
// Sensor configuration table
// Add or remove rows to change which sensors are explicitly mapped.
// address:      I2C address set by jumpers on the Trill Flex board
//               (0x48 = no jumpers, 0x49 = A0, 0x4A = A1, 0x4B = A0+A1, etc.)
// midiCC:       MIDI CC number to send
// doubleTapMute: if true, two quick taps mutes/unmutes this sensor
// ============================================================
struct ChannelConfig {
  uint8_t address;
  int midiCC;
  bool doubleTapMute;
};

const ChannelConfig CHANNEL_CONFIGS[] = {
  { 0x48, 13, false },  // sensor 1
  { 0x49, 11, false },  // sensor 2 — expression
  { 0x4A, 12, false },  // sensor 3
};

const int NUM_CONFIGURED = sizeof(CHANNEL_CONFIGS) / sizeof(CHANNEL_CONFIGS[0]);
const int MAX_CHANNELS   = 8;  // Trill Flex supports 8 unique I2C addresses (0x48–0x4F)

// ============================================================
// Runtime state for each active sensor channel
// ============================================================
struct TrillChannel {
  Trill trill;
  int midiCC;
  bool doubleTapMute;
  int lastVal          = -1;   // last CC value sent (used to suppress duplicate sends)
  int savedVal         = 64;   // CC value saved when muted, restored on unmute
  bool muted           = false;
  bool wasTouching     = false;
  unsigned long lastTapTime    = 0;  // timestamp of previous tap release (for double-tap)
  unsigned long touchStartTime = 0;  // timestamp when current touch began
};

TrillChannel channels[MAX_CHANNELS];
int activeChannels = 0;  // how many sensors successfully initialized at runtime

// ============================================================
// Initialize a single Trill Flex sensor at the given I2C address.
// Returns true on success, false if nothing responds at that address.
// ============================================================
bool setupChannel(TrillChannel &ch, uint8_t address) {
  if (ch.trill.setup(Trill::TRILL_FLEX, address) != 0) return false;

  ch.trill.setMode(Trill::CENTROID);  // report touch position, not raw data
  delay(100);
  ch.trill.setPrescaler(4);           // sensitivity tuning (higher = more sensitive)
  delay(200);
  ch.trill.updateBaseline();          // calibrate to current resting state
  delay(200);
  return true;
}

// ============================================================
// Scan I2C addresses 0x48–0x4F for any Trill Flex sensor
// that hasn't already been claimed by another channel.
// Returns the address if found, 0 if nothing available.
// ============================================================
uint8_t scanForTrill(uint8_t* claimed, int numClaimed) {
  for (uint8_t addr = 0x48; addr <= 0x4F; addr++) {
    // Skip addresses already in use
    bool skip = false;
    for (int i = 0; i < numClaimed; i++) {
      if (claimed[i] == addr) { skip = true; break; }
    }
    if (skip) continue;

    // Probe the address — endTransmission returns 0 if a device ACKs
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) return addr;
  }
  return 0;
}

// ============================================================
// Read one sensor and send MIDI CC if position has changed.
// Also handles double-tap mute/unmute if enabled for this channel.
// ============================================================
void processChannel(TrillChannel &ch) {
  ch.trill.read();
  bool touching      = ch.trill.getNumTouches() > 0;
  unsigned long now  = millis();

  // Detect the start of a new touch
  if (touching && !ch.wasTouching) {
    ch.touchStartTime = now;
  }

  // Detect touch release — evaluate for tap/double-tap
  if (!touching && ch.wasTouching) {
    unsigned long touchDuration = now - ch.touchStartTime;
    bool isTap = (touchDuration >= minTapDuration && touchDuration <= maxTapDuration);

    if (isTap && ch.doubleTapMute) {
      if (now - ch.lastTapTime < doubleTapWindow) {
        // Second tap within window — toggle mute
        ch.muted = !ch.muted;
        if (ch.muted) {
          ch.savedVal = ch.lastVal;
          MIDI.sendControlChange(ch.midiCC, 0, 1);           // send 0 to mute
        } else {
          MIDI.sendControlChange(ch.midiCC, ch.savedVal, 1); // restore saved value
          ch.lastVal = ch.savedVal;
        }
      }
      ch.lastTapTime = now;
    }
  }

  ch.wasTouching = touching;

  // Send CC position data while actively touching and not muted
  if (touching && !ch.muted) {
    int location = ch.trill.touchLocation(0);
    // Map raw sensor range to MIDI 0–127
    // 3712 is the maximum raw value for Trill Flex in centroid mode
    int val = constrain(map(location, 0, 3712, 0, 127), 0, 127);

    // Only send if value has moved enough to avoid jitter spam
    if (abs(val - ch.lastVal) > 1) {
      MIDI.sendControlChange(ch.midiCC, val, 1);
      ch.lastVal = val;
    }
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  TinyUSBDevice.setProductDescriptor("FlexSlider");
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(13, OUTPUT);  // LED: lights when any sensor is being touched

  Wire.begin();  // initialize I2C before any scanning or sensor setup

  uint8_t claimedAddresses[MAX_CHANNELS] = {};  // track which addresses are in use
  int numClaimed = 0;

  // --- Phase 1: initialize explicitly configured sensors ---
  // If a configured address isn't found, scan for any available sensor as fallback
  // and assign it to CC 7 (Volume) so it's visible in MIDI Monitor.
  for (int i = 0; i < NUM_CONFIGURED; i++) {
    uint8_t addr = CHANNEL_CONFIGS[i].address;
    claimedAddresses[numClaimed++] = addr;

    channels[activeChannels].midiCC        = CHANNEL_CONFIGS[i].midiCC;
    channels[activeChannels].doubleTapMute = CHANNEL_CONFIGS[i].doubleTapMute;

    if (!setupChannel(channels[activeChannels], addr)) {
      // Configured address not responding — try to find any unclaimed sensor
      uint8_t found = scanForTrill(claimedAddresses, numClaimed - 1);
      if (found != 0) {
        claimedAddresses[numClaimed - 1] = found;  // replace claimed entry with actual address
        if (setupChannel(channels[activeChannels], found)) {
          channels[activeChannels].midiCC        = 7;     // CC 7 = Volume fallback
          channels[activeChannels].doubleTapMute = false;
        } else {
          continue;  // found address but setup failed — skip this slot
        }
      } else {
        continue;  // nothing found at all — skip this slot
      }
    }

    delay(2000);
    channels[activeChannels].trill.updateBaseline();
    activeChannels++;
  }

  // --- Phase 2: discover any remaining unclaimed sensors ---
  // Sensors not listed in CHANNEL_CONFIGS get auto-assigned CCs
  // starting at AUTO_CC_START, incrementing for each additional sensor found.
  int autoCC = AUTO_CC_START;

  for (uint8_t scan = 0x48; scan <= 0x4F && activeChannels < MAX_CHANNELS; scan++) {
    // Skip already claimed addresses
    bool skip = false;
    for (int c = 0; c < numClaimed; c++) {
      if (claimedAddresses[c] == scan) { skip = true; break; }
    }
    if (skip) continue;

    // Check if anything is present at this address
    Wire.beginTransmission(scan);
    if (Wire.endTransmission() != 0) continue;

    // Found one — initialize and assign next available auto CC
    channels[activeChannels].midiCC        = autoCC++;
    channels[activeChannels].doubleTapMute = false;

    if (setupChannel(channels[activeChannels], scan)) {
      claimedAddresses[numClaimed++] = scan;
      delay(2000);
      channels[activeChannels].trill.updateBaseline();
      activeChannels++;
    }
  }

  // Wait until USB MIDI is recognized by the host before entering loop
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

  // LED on pin 13 lights whenever any sensor is being touched
  digitalWrite(13, anyTouch ? HIGH : LOW);

  delay(20);      // ~50Hz poll rate — sufficient for smooth MIDI control
  MIDI.read();    // keep USB MIDI stack happy
}