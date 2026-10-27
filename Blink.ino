#include <Wire.h>
#include <RH_ASK.h>
#include <SPI.h>

// ========================================================
// TRANSMITTER ARDUINO (HELMET UNIT) PIN CONFIGURATION
// ========================================================
const int HEAD_IR_PIN       = 2;    // Helmet Presence IR (Head) -> Arduino D2
const int EYE_BLINK_PIN     = 3;    // Eye Blink / Drowsiness    -> Arduino D3
const int BUZZER_PIN        = 8;    // Buzzer (+)                -> Arduino D8
const int LED_PIN           = 9;    // Red Indicator LED         -> Arduino D9
const int RF_TX_PIN         = 12;   // RF Transmitter DATA       -> Arduino D12
const int MQ3_ANALOG_PIN    = A0;   // MQ-3 Analog (AO)          -> Arduino A0
// MPU-6050 Accelerometer: SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND

// Alcohol threshold: 650 (Sober limit set per user request)
const int ALCOHOL_THRESHOLD = 650;

// MPU-6050 I2C Address (default is 0x68)
const int MPU_ADDR = 0x68;
bool mpuAvailable = false;

// 3-Axis & Attitude Telemetry
float ax_g = 0.04, ay_g = -0.02, az_g = 0.99;
float currentGForce = 0.99;
float pitch_deg = 2.1, roll_deg = 0.8;

// Eye Blink Timing & Drowsiness Tracker
unsigned long eyeClosedStartTime = 0;
bool isDrowsy = false;

// Initialize RadioHead ASK driver (Speed: 2000 bps, TX on Pin 12)
RH_ASK rf_driver(2000, 11, RF_TX_PIN);

void initMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // Power Management register 1
  Wire.write(0);    // Wake up MPU-6050
  byte error = Wire.endTransmission();

  if (error == 0) {
    mpuAvailable = true;
    Serial.println(F("[MPU-6050] Initialized successfully at I2C address 0x68."));
  } else {
    mpuAvailable = false;
    Serial.println(F("[MPU-6050] Not detected at 0x68 (Head IR + Eye Blink + MQ-3 active)."));
  }
}

bool readMPU() {
  if (!mpuAvailable) return false;

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Starting at register 0x3B (ACCEL_XOUT_H)
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(MPU_ADDR, 6, true);
  if (Wire.available() < 6) return false;

  int16_t raw_ax = Wire.read() << 8 | Wire.read();
  int16_t raw_ay = Wire.read() << 8 | Wire.read();
  int16_t raw_az = Wire.read() << 8 | Wire.read();

  // Convert raw values to G (scale is 16384 LSB/g)
  ax_g = (float)raw_ax / 16384.0;
  ay_g = (float)raw_ay / 16384.0;
  az_g = (float)raw_az / 16384.0;
  currentGForce = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

  // Compute live pitch (nod) and roll (lean) in degrees
  pitch_deg = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 180.0 / 3.14159265;
  roll_deg  = atan2(ay_g, az_g) * 180.0 / 3.14159265;

  // Impact threshold: > 2.6g or severe free-fall (< 0.25g) indicates a crash
  if (currentGForce > 2.6 || currentGForce < 0.25) {
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  delay(150);

  // Configure digital I/O
  pinMode(HEAD_IR_PIN, INPUT_PULLUP);
  pinMode(EYE_BLINK_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Hardware boot self-test: Red LED & Buzzer pulse for 1 second
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(900);
  digitalWrite(LED_PIN, LOW);

  Serial.println(F("\n================================================================================"));
  Serial.println(F("            AEGISHELM SMART HELMET SYSTEM - TRANSMITTER (COM7)                 "));
  Serial.println(F("================================================================================"));
  Serial.println(F("PINS: Head IR->D2 | Eye Blink->D3 | MQ3->A0 | Buzzer->D8 | Red LED->D9 | RF TX->D12 | MPU->A4/A5"));
  Serial.print(F("Alcohol Trigger Threshold: "));
  Serial.println(ALCOHOL_THRESHOLD);

  // Initialize MPU-6050
  initMPU();

  // Initialize RF Transmitter
  if (!rf_driver.init()) {
    Serial.println(F("ERROR: RF Transmitter failed on Pin D12!"));
  } else {
    Serial.println(F("RF Transmitter Ready on Pin D12 at 2000 bps."));
  }

  Serial.println(F("--------------------------------------------------------------------------------"));
  Serial.println(F("LIVE TELEMETRY STREAM STARTED:"));
  Serial.println(F("================================================================================"));
}

void loop() {
  digitalWrite(BUZZER_PIN, LOW);

  // 1. Read Helmet Presence IR (Pin D2) - Head Wear Verification
  int headVal = digitalRead(HEAD_IR_PIN);
  bool headDetected = (headVal == LOW); // Active-LOW: obstacle reflects IR into detector

  // 2. Read Eye Blink / Drowsiness Sensor (Pin D3)
  int eyeVal = digitalRead(EYE_BLINK_PIN);
  bool eyeClosed = (eyeVal == LOW); // Active-LOW: eyelid reflection triggers closed state

  if (eyeClosed) {
    if (eyeClosedStartTime == 0) {
      eyeClosedStartTime = millis();
    } else {
      unsigned long duration = millis() - eyeClosedStartTime;
      // Prolonged eye closure > 1.5 seconds triggers Drowsiness / Microsleep
      if (duration >= 1500) {
        isDrowsy = true;
      }
    }
  } else {
    eyeClosedStartTime = 0;
    isDrowsy = false;
  }

  // 3. Read MQ-3 Alcohol Sensor (Pin A0)
  int alcoholLevel = analogRead(MQ3_ANALOG_PIN);
  bool alcoholDetected = (alcoholLevel >= ALCOHOL_THRESHOLD);

  // 4. Read MPU-6050 3-Axis & Attitude (A4/A5)
  bool accident = readMPU();

  // 5. Determine System State & Priority
  // Priority: CRASH > ALCOHOL > DROWSY > NO_HELMET > SAFE
  const char *rfState = "NO_HELMET";
  if (accident) {
    rfState = "CRASH";
  } else if (alcoholDetected) {
    rfState = "ALCOHOL";
  } else if (isDrowsy) {
    rfState = "DROWSY";
  } else if (!headDetected) {
    rfState = "NO_HELMET";
  } else {
    rfState = "SAFE";
  }

  // 6. Transmit RF Packet with full motion telemetry (format: STATE:ax,ay,az,pitch,roll,gForce)
  char rfPacket[48];
  snprintf(rfPacket, sizeof(rfPacket), "%s:%.2f,%.2f,%.2f,%.1f,%.1f,%.2f",
           rfState, ax_g, ay_g, az_g, pitch_deg, roll_deg, currentGForce);

  rf_driver.send((uint8_t *)rfPacket, strlen(rfPacket));
  rf_driver.waitPacketSent(80);

  // 7. Clear, Unified Serial Telemetry Stream on COM7
  Serial.print(F("[HELMET] Head: "));
  Serial.print(headDetected ? F("OK   ") : F("OPEN "));
  Serial.print(F(" | Eye: "));
  if (isDrowsy) {
    Serial.print(F("DROWSY! "));
  } else if (eyeClosed) {
    Serial.print(F("CLOSED  "));
  } else {
    Serial.print(F("AWAKE   "));
  }
  Serial.print(F(" | Alc: "));
  if (alcoholLevel < 100) Serial.print(F(" "));
  if (alcoholLevel < 10)  Serial.print(F(" "));
  Serial.print(alcoholLevel);
  Serial.print(alcoholDetected ? F(" [ALERT!] ") : F(" [CLEAN]  "));
  Serial.print(F(" | MPU: "));
  if (mpuAvailable) {
    Serial.print(currentGForce, 2);
    Serial.print(F("G (X:"));
    Serial.print(ax_g, 2);
    Serial.print(F(" Y:"));
    Serial.print(ay_g, 2);
    Serial.print(F(" Z:"));
    Serial.print(az_g, 2);
    Serial.print(F(" P:"));
    Serial.print(pitch_deg, 1);
    Serial.print(F("° R:"));
    Serial.print(roll_deg, 1);
    Serial.print(F("°) "));
  } else {
    Serial.print(F("N/A "));
  }
  Serial.print(F("| SEND -> ["));
  Serial.print(rfState);
  Serial.println(F("]"));

  // 8. Visual & Acoustic Indicators on Helmet Unit
  if (accident) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  } else if (alcoholDetected) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
  } else if (isDrowsy) {
    // Pulse buzzer & flash LED on drowsiness to wake driver up
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(120);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  } else if (headDetected) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  } else {
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
