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

// Alcohol threshold: Clean air is ~250. Exceeding 450 triggers ALCOHOL alert.
const int ALCOHOL_THRESHOLD = 450;

// Initialize RadioHead ASK driver (Speed: 2000 bps, TX on Pin 12)
RH_ASK rf_driver(2000, 11, RF_TX_PIN);

void setup() {
  Serial.begin(9600);
  delay(200);

  // Configure digital I/O
  pinMode(IR_SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(IR_SENSOR_2_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // --- HARDWARE BOOT TEST ---
  // Force Red LED ON for 1 second to confirm wiring & LED polarity
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(900);
  digitalWrite(LED_PIN, LOW);

  Serial.println("==================================================");
  Serial.println("Arduino 1 (Helmet Transmitter) Running");
  Serial.println("Pins: IR1->D2 | IR2->D3 | MQ3->A0 | Buzzer->D8 | Red LED->D9 | RF->D12");
  Serial.print("Alcohol Threshold: ");
  Serial.println(ALCOHOL_THRESHOLD);
  Serial.println("==================================================");

  // Initialize RF Transmitter
  if (!rf_driver.init()) {
    Serial.println("Warning: RF Transmitter init failed!");
  } else {
    Serial.println("RF Transmitter Initialized on Pin D12.");
  }

  Serial.println("TRANSMITTER ACTIVE. STREAMING DATA BELOW:");
  Serial.println("==================================================");
}

void loop() {
  digitalWrite(BUZZER_PIN, LOW);

  // 1. Read Dual IR Sensors & Alcohol Sensor
  int ir1State = digitalRead(IR_SENSOR_1_PIN);
  int ir2State = digitalRead(IR_SENSOR_2_PIN);
  int alcoholLevel = analogRead(MQ3_ANALOG_PIN);

  // Active-low IR detection: LOW when obstacle reflects IR
  bool ir1Detected     = (ir1State == LOW);
  bool ir2Detected     = (ir2State == LOW);
  bool helmetWorn      = (ir1Detected && ir2Detected); // Both head + strap
  bool alcoholDetected = (alcoholLevel >= ALCOHOL_THRESHOLD);

  // 2. Determine System State & Transmit via RF
  const char *rfMsg = "NO_HELMET";

  if (alcoholDetected) {
    rfMsg = "ALCOHOL";
  } else if (!helmetWorn) {
    rfMsg = "NO_HELMET";
  } else {
    rfMsg = "SAFE"; // Helmet ON, Strap ON, No Alcohol
  }

  // Transmit RF packet with 100ms safe timeout (prevents any freeze)
  rf_driver.send((uint8_t *)rfMsg, strlen(rfMsg));
  rf_driver.waitPacketSent(100);

  // 3. Serial Monitor Output
  Serial.print("IR1: ");
  Serial.print(ir1Detected ? "DETECTED " : "CLEAR    ");
  Serial.print("| IR2: ");
  Serial.print(ir2Detected ? "DETECTED " : "CLEAR    ");
  Serial.print("| Alc: ");
  if (alcoholLevel < 100) Serial.print(" ");
  if (alcoholLevel < 10)  Serial.print(" ");
  Serial.print(alcoholLevel);
  if (alcoholDetected) {
    Serial.print(" [ALCOHOL ALERT]");
  } else {
    Serial.print(" [NORMAL]       ");
  }
  Serial.print(" | RF: [");
  Serial.print(rfMsg);
  Serial.print("] | Red LED: ");

  // 4. Red LED Indication on Helmet Unit (Pin D9)
  // - Alcohol Detected: SOLID ON
  // - Helmet Worn (or either IR active): BLINKS
  // - Clear / Off: OFF
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
