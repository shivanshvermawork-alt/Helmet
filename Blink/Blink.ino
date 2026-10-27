#include <Wire.h>
#include <RH_ASK.h>
#include <SPI.h>

// ========================================================
// TRANSMITTER ARDUINO (HELMET UNIT) PIN CONFIGURATION
// ========================================================
const int IR_SENSOR_1_PIN   = 2;    // IR Sensor 1 (Head)       -> Arduino D2
const int IR_SENSOR_2_PIN   = 3;    // IR Sensor 2 (Strap)      -> Arduino D3
const int BUZZER_PIN        = 8;    // Buzzer (+)               -> Arduino D8
const int LED_PIN           = 9;    // Red Indicator LED        -> Arduino D9
const int RF_TX_PIN         = 12;   // RF Transmitter DATA      -> Arduino D12
const int MQ3_ANALOG_PIN    = A0;   // MQ-3 Analog (AO)         -> Arduino A0
// MPU-6050 Accelerometer: SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND

// Alcohol threshold: 650 (Sober limit set per user request)
const int ALCOHOL_THRESHOLD = 650;

// MPU-6050 I2C Address (default is 0x68)
const int MPU_ADDR = 0x68;
bool mpuAvailable = false;
float currentGForce = 1.0;

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
    Serial.println(F("[MPU-6050] Not detected at 0x68 (Dual IR + MQ-3 active)."));
  }
}

bool checkAccident() {
  if (!mpuAvailable) return false;

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Starting at register 0x3B (ACCEL_XOUT_H)
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(MPU_ADDR, 6, true);
  if (Wire.available() < 6) return false;

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();

  // Convert raw values to g's (scale is 16384 LSB/g)
  currentGForce = sqrt((float)ax * ax + (float)ay * ay + (float)az * az) / 16384.0;

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
  pinMode(IR_SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(IR_SENSOR_2_PIN, INPUT_PULLUP);
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
  Serial.println(F("            AEGISHELM SMART HELMET SYSTEM - TRANSMITTER (COM5)                 "));
  Serial.println(F("================================================================================"));
  Serial.println(F("PINS: IR1->D2 | IR2->D3 | MQ3->A0 | Buzzer->D8 | Red LED->D9 | RF TX->D12 | MPU->A4/A5"));
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

  // 1. Read All Input Sensors
  int ir1State = digitalRead(IR_SENSOR_1_PIN);
  int ir2State = digitalRead(IR_SENSOR_2_PIN);
  int alcoholLevel = analogRead(MQ3_ANALOG_PIN);
  bool accident = checkAccident();

  // Active-low IR detection: LOW when obstacle reflects IR
  bool ir1Detected     = (ir1State == LOW);
  bool ir2Detected     = (ir2State == LOW);
  bool helmetWorn      = (ir1Detected && ir2Detected); // Both head + strap
  bool alcoholDetected = (alcoholLevel >= ALCOHOL_THRESHOLD);

  // 2. Determine System State & Priority
  // Priority: CRASH > ALCOHOL > NO_HELMET > SAFE
  const char *rfMsg = "NO_HELMET";

  if (accident) {
    rfMsg = "CRASH";
  } else if (alcoholDetected) {
    rfMsg = "ALCOHOL";
  } else if (!helmetWorn) {
    rfMsg = "NO_HELMET";
  } else {
    rfMsg = "SAFE"; // Helmet ON, Strap ON, No Alcohol, No Crash
  }

  // Transmit RF packet with 100ms safe timeout
  rf_driver.send((uint8_t *)rfMsg, strlen(rfMsg));
  rf_driver.waitPacketSent(100);

  // 3. Clear, Unified Serial Output Showing ALL Input Streams
  Serial.print(F("[HELMET] IR1(Head): "));
  Serial.print(ir1Detected ? F("OK   ") : F("OPEN "));
  Serial.print(F(" | IR2(Strap): "));
  Serial.print(ir2Detected ? F("OK   ") : F("OPEN "));
  Serial.print(F(" | MQ3(Alc): "));
  if (alcoholLevel < 100) Serial.print(F(" "));
  if (alcoholLevel < 10)  Serial.print(F(" "));
  Serial.print(alcoholLevel);
  Serial.print(alcoholDetected ? F(" [ALERT!] ") : F(" [CLEAN]  "));
  Serial.print(F(" | MPU: "));
  if (mpuAvailable) {
    Serial.print(currentGForce, 2);
    Serial.print(accident ? F("G [CRASH!] ") : F("G [NORMAL] "));
  } else {
    Serial.print(F("N/A [NO MPU]   "));
  }
  Serial.print(F(" | SEND -> ["));
  Serial.print(rfMsg);
  Serial.println(F("]"));

  // 4. Indicator Outputs on Helmet Unit
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
  } else if (ir1Detected || ir2Detected) {
    digitalWrite(LED_PIN, HIGH);
    delay(120);
    digitalWrite(LED_PIN, LOW);
    delay(120);
  } else {
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}
