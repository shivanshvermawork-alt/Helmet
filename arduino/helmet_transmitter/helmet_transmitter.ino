#include <RH_ASK.h>
#include <SPI.h>
#include <Wire.h>

// ========================================================
// TRANSMITTER ARDUINO (HELMET UNIT) PIN CONFIGURATION
// ========================================================
const int IR_SENSOR_1_PIN   = 2;    // IR Sensor 1 (Head Presence) -> Arduino D2
const int IR_SENSOR_2_PIN   = 3;    // IR Sensor 2 (Strap Buckle)  -> Arduino D3
const int BUZZER_PIN        = 8;    // Local Helmet Buzzer (+)     -> Arduino D8
const int LED_PIN           = 9;    // Red Indicator LED           -> Arduino D9
const int RF_TX_PIN         = 12;   // 433MHz RF Transmitter DATA  -> Arduino D12
const int MQ3_ANALOG_PIN    = A0;   // MQ-3 Alcohol Analog (AO)    -> Arduino A0

// Optional MPU-6050 3-Axis Accelerometer on I2C (SDA -> A4, SCL -> A5)
const int MPU_ADDR          = 0x68;
bool hasMPU                 = false;

// Safety thresholds
const int ALCOHOL_THRESHOLD = 450;
const float CRASH_G_THRESHOLD = 3.2; // G-force spike threshold (> 3.2G considered impact)

// Initialize RadioHead ASK driver (Speed: 2000 bps, TX on Pin 12)
RH_ASK rf_driver(2000, 11, RF_TX_PIN);

// MPU-6050 variables
float ax_g = 0.0, ay_g = 0.0, az_g = 1.0;
float total_g = 1.0;
float pitch = 0.0, roll = 0.0;
bool crashDetected = false;

void initMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() == 0) {
    // Wake up MPU-6050
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); // PWR_MGMT_1 register
    Wire.write(0);    // Wake up
    Wire.endTransmission(true);
    hasMPU = true;
    Serial.println("MPU-6050 Detected on I2C (A4/A5). Motion telemetry enabled.");
  } else {
    hasMPU = false;
    Serial.println("MPU-6050 Not Found. Running dual-IR + MQ-3 baseline mode.");
  }
}

void readMPU() {
  if (!hasMPU) return;

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  if (Wire.available() >= 6) {
    int16_t raw_ax = Wire.read() << 8 | Wire.read();
    int16_t raw_ay = Wire.read() << 8 | Wire.read();
    int16_t raw_az = Wire.read() << 8 | Wire.read();

    ax_g = (float)raw_ax / 16384.0;
    ay_g = (float)raw_ay / 16384.0;
    az_g = (float)raw_az / 16384.0;

    total_g = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    pitch = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 180.0 / PI;
    roll  = atan2(ay_g, az_g) * 180.0 / PI;

    if (total_g > CRASH_G_THRESHOLD) {
      crashDetected = true;
    } else {
      crashDetected = false;
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(200);

  // Configure digital I/O
  pinMode(IR_SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(IR_SENSOR_2_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // --- HARDWARE BOOT TEST ---
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(900);
  digitalWrite(LED_PIN, LOW);

  Serial.println("==================================================");
  Serial.println("AegisHelm Helmet Transmitter Unit Active");
  Serial.println("Pins: IR1->D2 | IR2->D3 | MQ3->A0 | Buzzer->D8 | Red LED->D9 | RF->D12");
  Serial.print("Alcohol Threshold: ");
  Serial.println(ALCOHOL_THRESHOLD);

  // Probe MPU-6050
  initMPU();

  // Initialize RF Transmitter
  if (!rf_driver.init()) {
    Serial.println("Warning: RF Transmitter init failed on Pin 12!");
  } else {
    Serial.println("RF Transmitter Initialized on Pin D12 (2000 bps).");
  }
  Serial.println("==================================================");
}

void loop() {
  digitalWrite(BUZZER_PIN, LOW);

  // 1. Read Sensors
  int ir1State = digitalRead(IR_SENSOR_1_PIN);
  int ir2State = digitalRead(IR_SENSOR_2_PIN);
  int alcoholLevel = analogRead(MQ3_ANALOG_PIN);

  bool ir1Detected     = (ir1State == LOW);
  bool ir2Detected     = (ir2State == LOW);
  bool helmetWorn      = (ir1Detected && ir2Detected); // Both head + strap required
  bool alcoholDetected = (alcoholLevel >= ALCOHOL_THRESHOLD);

  // 2. Read Accelerometer
  readMPU();

  // 3. Determine System State & RF Message
  const char *rfMsg = "NO_HELMET";

  if (alcoholDetected) {
    rfMsg = "ALCOHOL";
  } else if (!helmetWorn) {
    rfMsg = "NO_HELMET";
  } else {
    rfMsg = "SAFE"; // Helmet ON, Strap ON, Clean Air
  }

  // Transmit RF packet with 100ms safe timeout
  rf_driver.send((uint8_t *)rfMsg, strlen(rfMsg));
  rf_driver.waitPacketSent(100);

  // 4. Serial Monitor Output
  Serial.print("IR1: ");
  Serial.print(ir1Detected ? "DETECTED " : "CLEAR    ");
  Serial.print("| IR2: ");
  Serial.print(ir2Detected ? "DETECTED " : "CLEAR    ");
  Serial.print("| Alc: ");
  Serial.print(alcoholLevel);
  if (alcoholDetected) {
    Serial.print(" [ALCOHOL ALERT]");
  } else {
    Serial.print(" [NORMAL]       ");
  }
  Serial.print(" | RF: [");
  Serial.print(rfMsg);
  Serial.print("] | Red LED: ");

  // 5. Red LED Indication on Helmet Unit (Pin D9)
  if (alcoholDetected) {
    Serial.println("SOLID ON");
    digitalWrite(LED_PIN, HIGH);
    delay(300);
  } else if (ir1Detected || ir2Detected) {
    Serial.println("BLINKING");
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  } else {
    Serial.println("OFF");
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
}
