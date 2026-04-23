#include <Wire.h>
#include <Trill.h>

Trill trill;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(500);
  
  int result = trill.setup(Trill::TRILL_FLEX, 0x48);
  if (result == 0) {
    Serial.println("Trill Flex found!");
    trill.setMode(Trill::DIFF);
    delay(100);
    trill.updateBaseline();
    Serial.println("Ready");
  } else {
    Serial.print("Not found, error: ");
    Serial.println(result);
  }
}

void loop() {
  trill.requestRawData();
  int maxVal = 0;
  int maxIdx = 0;
  int count = 0;
  while (trill.rawDataAvailable() > 0) {
    int val = trill.rawDataRead();
    if (val > maxVal) {
      maxVal = val;
      maxIdx = count;
    }
    count++;
  }
  if (maxVal > 100) {  // only print if significant touch
    Serial.print("Touch at channel: ");
    Serial.print(maxIdx);
    Serial.print("  Value: ");
    Serial.println(maxVal);
  }
  delay(50);
}