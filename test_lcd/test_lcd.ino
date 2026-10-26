#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Standard I2C address is 0x27 for PCF8574T (or 0x3F for PCF8574AT)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  Serial.println("--- LCD Hello World Test ---");

  // 1. Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // 2. Print initial test message
  lcd.setCursor(0, 0);
  lcd.print("SMART HELMET SYS");
  lcd.setCursor(0, 1);
  lcd.print("LCD TEST: OK!");

  Serial.println("Displayed: SMART HELMET SYS / LCD TEST: OK!");
}

void loop() {
  // Blink backlight every 3 seconds to prove communication is active
  delay(3000);
  lcd.noBacklight();
  delay(500);
  lcd.backlight();
}
