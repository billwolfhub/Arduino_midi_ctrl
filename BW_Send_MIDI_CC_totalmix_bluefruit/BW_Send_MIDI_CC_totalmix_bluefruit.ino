void setup() {
  usbMIDI.begin();
}

void loop() {
  int val = analogRead(A0);
  val = map(val, 0, 1023, 0, 127);
  usbMIDI.sendControlChange(7, val, 1);
  delay(20);
  
  while (usbMIDI.read()) {}
}