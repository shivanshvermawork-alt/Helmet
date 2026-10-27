#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RH_ASK.h>
#include <SPI.h> // Required by RadioHead even if not used directly

// ========================================================
// PIN CONFIGURATION (RECEIVER / BIKE IGNITION UNIT)
// ========================================================
// RF Receiver DATA -> Arduino D2 (or D11 if preferred)
// Buzzer (+)       -> Arduino D3
// Relay Module IN  -> Arduino D8 (Active LOW)
// Green LED (+)    -> Arduino D9 (Ignition indicator)
// I2C LCD SDA      -> Arduino A4
// I2C LCD SCL      -> Arduino A5
const int RF_RX_PIN    = 2;
const int RF_TX_UNUSED = 12; // Unused TX pin required by constructor
const int BUZZER_PIN   = 3;
const int RELAY_PIN    = 8;  // Active LOW relay module
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

// Telemetry state variables
int helmetState   = 0; // 1 = Worn, 0 = Off
int alcoholState  = 0; // 1 = Detected, 0 = Clean
int drowsyState   = 0; // 1 = Drowsy, 0 = Alert
int crashState    = 0; // 1 = Crash/Fall, 0 = Normal
float ax_val = 0.02, ay_val = 0.01, az_val = 0.99;
float gForce_val = 1.00;
float pitch_val = 0.0, roll_val = 0.0;

bool engineRunning = false;
bool manualLockOverride = false;
unsigned long lastTelemetryStream = 0;

// ========================================================
// HELPER FUNCTIONS
// ========================================================
void setEngine(bool enable) {
  if (enable && !manualLockOverride) {
    digitalWrite(RELAY_PIN, RELAY_ON);  // Engine ignition ON
    digitalWrite(LED_PIN, HIGH);        // Green indicator ON
    engineRunning = true;
  } else {
    digitalWrite(RELAY_PIN, RELAY_OFF); // Engine ignition OFF
    digitalWrite(LED_PIN, LOW);         // Green indicator OFF
    engineRunning = false;
  }
}

bool hasPhysicalLCD = false;
String lcdLine1 = "SMART HELMET SYS";
String lcdLine2 = "RECEIVER READY..";

// Redraws LCD only when state text changes (prevents flicker and I2C bus traffic)
void displayStatus(const char* line1, const char* line2) {
  lcdLine1 = String(line1);
  lcdLine2 = String(line2);

  if (hasPhysicalLCD) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

// Streams JSON Telemetry to PC/Web Gateway over USB Serial (2Hz / 500ms)
void streamTelemetryJSON() {
  if (millis() - lastTelemetryStream < 500) return;
  lastTelemetryStream = millis();

  Serial.print(F("{\"helmet\":"));
  Serial.print(helmetState);
  Serial.print(F(",\"alcohol\":"));
  Serial.print(alcoholState);
  Serial.print(F(",\"drowsy\":"));
  Serial.print(drowsyState);
  Serial.print(F(",\"crash\":"));
  Serial.print(crashState);
  Serial.print(F(",\"ax\":")); Serial.print(ax_val, 2);
  Serial.print(F(",\"ay\":")); Serial.print(ay_val, 2);
  Serial.print(F(",\"az\":")); Serial.print(az_val, 2);
  Serial.print(F(",\"gForce\":")); Serial.print(gForce_val, 2);
  Serial.print(F(",\"pitch\":")); Serial.print(pitch_val, 1);
  Serial.print(F(",\"roll\":")); Serial.print(roll_val, 1);
  Serial.print(F(",\"relay\":")); Serial.print(engineRunning ? 1 : 0);
  Serial.print(F(",\"override\":")); Serial.print(manualLockOverride ? 1 : 0);
  Serial.print(F(",\"speedKmh\":")); Serial.print(engineRunning ? 42 : 0);
  Serial.print(F(",\"lcd1\":\"")); Serial.print(lcdLine1); Serial.print(F("\""));
  Serial.print(F(",\"lcd2\":\"")); Serial.print(lcdLine2); Serial.print(F("\""));
  Serial.println(F("}"));
}

// Listens for Remote Killswitch commands ("LOCK" / "UNLOCK") from Web Dashboard
void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "LOCK") {
      manualLockOverride = true;
      setEngine(false);
      displayStatus("REMOTE OVERRIDE", "IGNITION CUT!");
      Serial.println(F("{\"status\":\"LOCKED_BY_REMOTE\"}"));
    } else if (cmd == "UNLOCK") {
      manualLockOverride = false;
      if (lastState == "SAFE") {
        setEngine(true);
        displayStatus("HELMET: SAFE OK", "ENGINE: STARTED");
      }
      Serial.println(F("{\"status\":\"UNLOCKED_BY_REMOTE\"}"));
    }
  }
}

// ========================================================
// SETUP
// ========================================================
void setup() {
  Serial.begin(9600);
  delay(100);

  // 1. Configure output pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Safety initial state: Engine locked, buzzer silent
  setEngine(false);
  digitalWrite(BUZZER_PIN, LOW);

  // 2. Safe I2C LCD Detection (Protects against broken/missing LCD freezing MCU)
  Wire.begin();
  Wire.beginTransmission(0x27);
  if (Wire.endTransmission() == 0) {
    hasPhysicalLCD = true;
    lcd.init();
    lcd.backlight();
    displayStatus("SMART HELMET SYS", "RECEIVER READY..");
  } else {
    Wire.beginTransmission(0x3F);
    if (Wire.endTransmission() == 0) {
      hasPhysicalLCD = true;
      lcd.init();
      lcd.backlight();
      displayStatus("SMART HELMET SYS", "RECEIVER READY..");
    } else {
      hasPhysicalLCD = false;
      Serial.println(F("[LCD] Physical LCD not found at 0x27/0x3F (Using Dashboard Mirror)."));
    }
  }

  // 3. Initialize RadioHead 433MHz RF Receiver
  if (!rf_driver.init()) {
    Serial.println(F("[RF] ERROR: RadioHead RF init failed on Pin D2!"));
  } else {
    Serial.println(F("[RF] Receiver Ready on Pin D2 (2000 bps)."));
  }

  Serial.println(F("\n================================================================================"));
  Serial.println(F("            AEGISHELM SMART HELMET SYSTEM - BIKE RECEIVER (COM4)                "));
  Serial.println(F("================================================================================"));
  Serial.println(F("PINS: RF RX->D2 | Relay->D8 | Green LED->D9 | Buzzer->D3 | LCD->A4/A5"));
  Serial.println(F("LISTENING FOR 433MHz HELMET TRANSMISSIONS ON PIN D2..."));
  Serial.println(F("================================================================================"));
}

// ========================================================
// MAIN LOOP
// ========================================================
void loop() {
  // Check for incoming commands from Web Dashboard
  checkSerialCommands();

  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN + 1];
  uint8_t buflen = RH_ASK_MAX_MESSAGE_LEN;

  // 1. Non-blocking check for received RF packet
  if (rf_driver.recv(buf, &buflen)) {
    lastPacketTime = millis();

    // Null-terminate received byte array to form a valid C-string
    buf[buflen] = '\0';
    String message = String((char*)buf);
    message.trim();

    // 1. Separate state and MPU motion payload if present (Format: STATE:ax,ay,az,pitch,roll,gForce)
    String stateStr = message;
    int colonIdx = message.indexOf(':');
    if (colonIdx > 0) {
      stateStr = message.substring(0, colonIdx);
      String mpuPayload = message.substring(colonIdx + 1);
      float ax = 0.0, ay = 0.0, az = 0.0, p = 0.0, r = 0.0, g = 0.0;
      int cnt = sscanf(mpuPayload.c_str(), "%f,%f,%f,%f,%f,%f", &ax, &ay, &az, &p, &r, &g);
      if (cnt >= 5) {
        ax_val = ax;
        ay_val = ay;
        az_val = az;
        pitch_val = p;
        roll_val = r;
        if (cnt >= 6) gForce_val = g;
      }
    }

    // Human-readable debug output for Serial Monitor
    Serial.print(F("[BIKE RX] RF: ["));
    Serial.print(stateStr);
    Serial.print(F("] | Relay: "));
    Serial.print(engineRunning ? F("ON (RUNNING) ") : F("OFF (LOCKED) "));
    Serial.print(F("| LCD: \""));
    Serial.print(lcdLine1);
    Serial.print(F("\" / \""));
    Serial.print(lcdLine2);
    Serial.print(F("\" | MPU: X:"));
    Serial.print(ax_val, 2);
    Serial.print(F(" Y:"));
    Serial.print(ay_val, 2);
    Serial.print(F(" Z:"));
    Serial.print(az_val, 2);
    Serial.print(F(" P:"));
    Serial.print(pitch_val, 1);
    Serial.print(F("° R:"));
    Serial.print(roll_val, 1);
    Serial.print(F("° G:"));
    Serial.print(gForce_val, 2);
    Serial.println();

    // 2. Handle System States
    if (stateStr == "SAFE") {
      helmetState  = 1;
      alcoholState = 0;
      drowsyState  = 0;
      crashState   = 0;

      if (lastState != "SAFE") {
        lastState = "SAFE";
        setEngine(true); // Relay ON, Green LED ON
        digitalWrite(BUZZER_PIN, LOW);
        displayStatus("HELMET: SAFE OK", "ENGINE: STARTED");
      }
    } 
    else if (stateStr == "DROWSY") {
      helmetState  = 1;
      alcoholState = 0;
      drowsyState  = 1;
      crashState   = 0;

      if (lastState != "DROWSY") {
        lastState = "DROWSY";
        displayStatus("DROWSINESS ALERT", "TAKE A BREAK!   ");
      }
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(60);
    }
    else if (stateStr == "ALCOHOL") {
      helmetState  = 1;
      alcoholState = 1;
      drowsyState  = 0;
      crashState   = 0;

      if (lastState != "ALCOHOL") {
        lastState = "ALCOHOL";
        setEngine(false); // Relay OFF, Green LED OFF
        displayStatus("ALCOHOL DETECTED", "ENGINE: LOCKED!");
      }
      // Alarm pattern: rapid beeps
      digitalWrite(BUZZER_PIN, HIGH);
      delay(80);
      digitalWrite(BUZZER_PIN, LOW);
      delay(60);
    } 
    else if (stateStr == "NO_HELMET") {
      helmetState  = 0;
      alcoholState = 0;
      drowsyState  = 0;
      crashState   = 0;

      if (lastState != "NO_HELMET") {
        lastState = "NO_HELMET";
        setEngine(false); // Relay OFF, Green LED OFF
        digitalWrite(BUZZER_PIN, LOW);
        displayStatus("WEAR HELMET &", "FASTEN STRAP!");
      }
    }
    else if (stateStr == "CRASH" || stateStr == "ACCIDENT") {
      crashState = 1;
      if (lastState != "CRASH") {
        lastState = "CRASH";
        setEngine(false); // Kill engine immediately on impact
        digitalWrite(BUZZER_PIN, HIGH);
        displayStatus("CRASH DETECTED! ", "SOS DISPATCHING ");
      }
    }
  }

  // 3. Signal Timeout Check: If no packet received within 3 seconds
  // 3. Signal Timeout Check: If no packet received within 3 seconds
  if (lastPacketTime > 0 && (millis() - lastPacketTime > SIGNAL_TIMEOUT_MS)) {
    if (lastState != "TIMEOUT") {
      lastState = "TIMEOUT";
      helmetState = 0;
      setEngine(false); // Kill engine if helmet goes out of range
      digitalWrite(BUZZER_PIN, LOW);
      displayStatus("SIGNAL LOST", "ENGINE STOPPED");
      Serial.println(F("[RECEIVER] RF Signal Lost (>3s) -> Engine Stopped."));
    }
  }

  // 4. Heartbeat when waiting for initial signal
  static unsigned long lastWaitHeartbeat = 0;
  if (lastPacketTime == 0 && (millis() - lastWaitHeartbeat >= 3000)) {
    lastWaitHeartbeat = millis();
    Serial.println(F("[RECEIVER] Listening on Pin D2... (Waiting for RF signal from Helmet)"));
  }

  delay(20);
}
