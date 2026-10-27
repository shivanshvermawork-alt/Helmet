#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // Wait for serial monitor to open
  Serial.println("\n--- Arduino Uno I2C Scanner ---");
  Serial.println("Scanning pins A4 (SDA) and A5 (SCL)...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("\nScanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("SUCCESS: I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);

      if (address == 0x27) Serial.println(" (PCF8574T standard LCD address)");
      else if (address == 0x3F) Serial.println(" (PCF8574AT alternate LCD address)");
      else Serial.println(" !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found.");
    Serial.println("-> Check: SDA to A4, SCL to A5, VCC to 5V, GND to GND.");
  } else {
    Serial.println("Scan complete.");
  }

  delay(3000); // Wait 3 seconds before next scan
}
