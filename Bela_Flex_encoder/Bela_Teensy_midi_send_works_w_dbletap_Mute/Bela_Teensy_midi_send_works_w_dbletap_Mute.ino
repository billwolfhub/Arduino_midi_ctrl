#include <Wire.h>
#include <Trill.h>

Trill trill;
int lastVal = -1;
int savedVal = 64;  // saves volume before mute
bool muted = false;

// double tap detection
unsigned long lastTapTime = 0;
bool wasTouching = false;
unsigned long touchStartTime = 0;
const unsigned long doubleTapWindow = 400;
const unsigned long minTapDuration = 20;
const unsigned long maxTapDuration = 300;

void setup() {
  usbMIDI.begin();
  
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
}

void loop() {
  trill.read();
  bool touching = trill.getNumTouches() > 0;
  unsigned long now = millis();

  if (touching && !wasTouching) {
    touchStartTime = now;
  }

  if (!touching && wasTouching) {
    unsigned long touchDuration = now - touchStartTime;
    if (touchDuration >= minTapDuration && touchDuration <= maxTapDuration) {
      if (now - lastTapTime < doubleTapWindow) {
        // double tap detected
        muted = !muted;
        if (muted) {
          savedVal = lastVal;  // save current volume
          usbMIDI.sendControlChange(7, 0, 1);
        } else {
          usbMIDI.sendControlChange(7, savedVal, 1);  // restore saved volume
          lastVal = savedVal;
        }
      }
      lastTapTime = now;
    }
  }

  wasTouching = touching;

  if (touching && !muted) {
    int location = trill.touchLocation(0);
    int val = map(location, 0, 3712, 0, 127);
    val = constrain(val, 0, 127);

    if (abs(val - lastVal) > 1) {
      usbMIDI.sendControlChange(7, val, 1);
      lastVal = val;
    }
  }

  delay(20);
  while (usbMIDI.read()) {}
}