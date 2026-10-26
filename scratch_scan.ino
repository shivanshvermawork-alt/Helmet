#include <Wire.h>

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(1000);
  Serial.println("=== I2C SCAN START ===");
  int count = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(addr, HEX);
      count++;
    }
  }
  if (count == 0) {
    Serial.println("No I2C devices found on this board!");
  }
  Serial.println("=== I2C SCAN DONE ===");
}

void loop() {
  delay(1000);
}
