int lastVal = -1;
int lastRaw = -1;

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

  // ignore large sudden jumps (finger release)
  if (lastRaw != -1 && abs(raw - lastRaw) > 100) {
    lastRaw = raw;
    delay(20);
    while (usbMIDI.read()) {}
    return;
  }

  lastRaw = raw;
  int val = map(raw, 0, 1023, 0, 127);
  val = constrain(val, 0, 127);

  if (abs(val - lastVal) > 1) {
    usbMIDI.sendControlChange(7, val, 1);
    lastVal = val;
  }

  delay(20);
  while (usbMIDI.read()) {}
}