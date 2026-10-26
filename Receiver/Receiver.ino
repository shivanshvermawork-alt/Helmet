#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RH_ASK.h>
#include <SPI.h> // Required by RadioHead even if not used directly

// ========================================================
// PIN CONFIGURATION (RECEIVER / BIKE IGNITION UNIT)
// ========================================================
// RF Receiver DATA -> Arduino D11 (RadioHead standard RX pin)
// Buzzer (+)       -> Arduino D3
// Relay Module IN  -> Arduino D8
// Green LED (+)    -> Arduino D9 (digitalWrite only; PWM disabled by Timer1)
// I2C LCD SDA      -> Arduino A4 (or dedicated SDA pin past AREF)
// I2C LCD SCL      -> Arduino A5 (or dedicated SCL pin past AREF)
const int RF_RX_PIN    = 11;
const int RF_TX_UNUSED = 12; // Unused TX pin required by constructor
const int BUZZER_PIN   = 3;
const int RELAY_PIN    = 8;
const int LED_PIN      = 9;

// Relay Active Level: 5V relay modules are ACTIVE LOW (LOW = Relay ON)
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

// ========================================================
// HARDWARE DRIVERS
// ========================================================
// PCF8574T default I2C address is 0x27 (change to 0x3F if using PCF8574AT)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RadioHead ASK driver (2000 bps, RX on Pin 11, dummy TX on Pin 12)
RH_ASK rf_driver(2000, RF_RX_PIN, RF_TX_UNUSED);

// State tracking & non-blocking timers
String lastState = "";
unsigned long lastPacketTime = 0;
const unsigned long SIGNAL_TIMEOUT_MS = 3000; // 3 sec timeout if helmet out of range

// ========================================================
// HELPER FUNCTIONS
// ========================================================
void setEngine(bool enable) {
  if (enable) {
    digitalWrite(RELAY_PIN, RELAY_ON);  // Engine ignition ON
    digitalWrite(LED_PIN, HIGH);        // Green indicator ON
  } else {
    digitalWrite(RELAY_PIN, RELAY_OFF); // Engine ignition OFF
    digitalWrite(LED_PIN, LOW);         // Green indicator OFF
  }
}

// Redraws LCD only when state text changes (prevents flicker and I2C bus traffic)
void displayStatus(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ========================================================
// SETUP
// ========================================================
void setup() {
  Serial.begin(9600);
  Serial.println("\n==========================================");
  Serial.println("  Smart Helmet System - Receiver (Bike)");
  Serial.println("==========================================");

  // 1. Configure output pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Safety initial state: Engine locked, buzzer silent
  setEngine(false);
  digitalWrite(BUZZER_PIN, LOW);

  // 2. Initialize I2C LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SMART HELMET SYS");
  lcd.setCursor(0, 1);
  lcd.print("RECEIVER READY..");
  Serial.println("LCD Initialized.");

  // 3. Initialize RadioHead 433MHz RF Receiver
  if (!rf_driver.init()) {
    Serial.println("ERROR: RadioHead RF driver init failed on Pin 11!");
  } else {
    Serial.println("RF Receiver listening on Pin 11 at 2000 bps.");
  }
}

// ========================================================
// MAIN LOOP
// ========================================================
void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  // 1. Non-blocking check for received RF packet
  if (rf_driver.recv(buf, &buflen)) {
    lastPacketTime = millis();

    // CRITICAL: Null-terminate received byte array to form a valid C-string
    buf[buflen] = '\0';
    String message = String((char*)buf);
    message.trim();

    Serial.print("RF Packet Received: [");
    Serial.print(message);
    Serial.println("]");

    // 2. Handle System States
    if (message == "SAFE") {
      if (lastState != "SAFE") {
        lastState = "SAFE";
        setEngine(true); // Relay ON, Green LED ON
        digitalWrite(BUZZER_PIN, LOW);
        displayStatus("HELMET: SAFE OK", "ENGINE: STARTED");
      }
    } 
    else if (message == "ALCOHOL") {
      if (lastState != "ALCOHOL") {
        lastState = "ALCOHOL";
        setEngine(false); // Relay OFF, Green LED OFF
        displayStatus("ALCOHOL DETECTED", "ENGINE: LOCKED!");
      }
      // Alarm pattern: rapid non-blocking beeps
      digitalWrite(BUZZER_PIN, HIGH);
      delay(80);
      digitalWrite(BUZZER_PIN, LOW);
      delay(60);
    } 
    else if (message == "NO_HELMET") {
      if (lastState != "NO_HELMET") {
        lastState = "NO_HELMET";
        setEngine(false); // Relay OFF, Green LED OFF
        digitalWrite(BUZZER_PIN, LOW);
        displayStatus("WEAR HELMET &", "FASTEN STRAP!");
      }
    }
  }

  // 3. Signal Timeout Check: If no packet received within 3 seconds
  if (lastPacketTime > 0 && (millis() - lastPacketTime > SIGNAL_TIMEOUT_MS)) {
    if (lastState != "TIMEOUT") {
      lastState = "TIMEOUT";
      setEngine(false); // Kill engine if helmet goes out of range
      digitalWrite(BUZZER_PIN, LOW);
      displayStatus("SIGNAL LOST", "ENGINE STOPPED");
      Serial.println("Warning: RF signal lost. Engine stopped.");
    }
  }

  // Tiny delay to yield MCU
  delay(10);
}
