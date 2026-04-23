#include <Wire.h>
#include <Trill.h>

Trill trill;
int lastVal = -1;

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
  
  if (trill.getNumTouches() > 0) {
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