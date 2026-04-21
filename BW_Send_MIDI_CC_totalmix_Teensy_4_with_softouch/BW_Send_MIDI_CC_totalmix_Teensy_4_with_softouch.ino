int lastVal = -1;

void setup() {
  usbMIDI.begin();
}

void loop() {
  int raw = 0;
  for (int i = 0; i < 32; i++) {
    raw += analogRead(A0);
    delay(1);
  }
  raw /= 32;

  int val = map(raw, 54, 973, 0, 127);
  val = constrain(val, 0, 127);

  if (abs(val - lastVal) > 2) {
    usbMIDI.sendControlChange(7, val, 1);
    lastVal = val;
  }

  delay(20);
  while (usbMIDI.read()) {}
}