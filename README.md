# AegisHelm — Smart Helmet Telemetry & Motorcycle Safety System
### Embedded IoT Motorcycle Protection & Safety Telemetry System

A full-stack IoT platform and embedded hardware firmware for the **Smart Helmet Based Motorcycle Protection and Monitoring Control System**, featuring dual-IR wear & strap verification, MQ-3 alcohol breath interlock, 433MHz RF wireless telemetry, an active-LOW ignition starter relay, an on-bike 16x2 I2C LCD, a 10Hz Node.js WebSocket serial gateway, and a real-time widescreen live cockpit dashboard.

---

## 📁 System Architecture & Directory Structure

```text
stitch_aegishelm_smart_motorcycle_dashboard/
├── Launch_Cockpit.cmd                          # 1-Click Windows launcher (starts server & opens browser)
├── aegishelm_live_cockpit_dashboard/
│   └── code.html                               # Widescreen Live Cockpit Dashboard (max-w-[1720px], Port 8080)
├── aegishelm_trip_logs_blackbox_scorecard/
│   └── code.html                               # Trip Logs & Blackbox Scorecard audit page (/trip-logs)
├── aegishelm_cockpit_interface/
│   └── DESIGN.md                               # UI Architecture & Design System specifications
├── arduino/
│   ├── Blink/
│   │   └── Blink.ino                           # Helmet Unit Firmware (Dual-IR head+strap, MQ-3, Buzzer, LED, RF TX)
│   ├── Receiver/
│   │   └── Receiver.ino                        # Bike Unit Firmware (RF RX, Relay D8, LED D9, Buzzer D3, LCD, JSON Serial)
│   ├── helmet_transmitter/
│   │   └── helmet_transmitter.ino              # Unified Helmet Firmware (with optional MPU-6050 auto-detection)
│   ├── bike_receiver/
│   │   └── bike_receiver.ino                   # Unified Bike Firmware (matching Receiver.ino)
│   ├── diagnostics/                            # Diagnostic & testing sketches
│   │   ├── i2c_scanner/
│   │   │   └── i2c_scanner.ino                 # Scans A4/A5 for I2C devices (0x27 / 0x3F)
│   │   ├── test_lcd/
│   │   │   └── test_lcd.ino                    # Verifies 16x2 I2C LCD display and backlight
│   │   └── Scan/
│   │       └── Scan.ino                        # Quick I2C bus scanner
│   └── tools/
│       └── monitor.ps1                         # PowerShell live COM port serial monitor tool
└── server/
    ├── server.js                               # Node.js Express + Socket.io + SerialPort gateway (Dual-mode parser)
    └── package.json
```

---

## ⚡ Hardware Pinout Connections

### 1. Transmitter Unit (Mounted on Helmet — Arduino Uno / Nano)
| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **Head IR Presence Sensor** | Pin `D2` | Active LOW (Detects head inside helmet via internal pullup) |
| **Strap IR Buckle Sensor** | Pin `D3` | Active LOW (Detects chin strap securely buckled) |
| **Alcohol Sensor (MQ-3 AO)** | Pin `A0` | Analog breath measurement (Clean air ~250, Threshold: 450) |
| **Local Helmet Buzzer (+)** | Pin `D8` | Alerts rider on alcohol detection or prolonged warning |
| **Red Status Indicator LED** | Pin `D9` | Solid ON on alcohol alert; blinks when helmet worn |
| **433MHz RF Transmitter DATA** | Pin `D12` | 2000 bps ASK modulation (`RH_ASK` / VirtualWire) |
| *(Optional)* **MPU-6050 SDA / SCL** | Pins `A4` / `A5` | Auto-detected for 3-axis G-force & tilt telemetry |

### 2. Receiver Unit (Mounted on Motorcycle — Arduino Uno)
| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **433MHz RF Receiver DATA** | Pin `D11` | Receives RF packets from helmet transmitter |
| **Starter Ignition Relay IN** | Pin `D8` | Controls bike ignition circuit (Active LOW: LOW = ON, HIGH = OFF) |
| **Green Ignition Status LED** | Pin `D9` | Indicates engine ignition circuit is armed and live |
| **Vehicle Warning Buzzer (+)** | Pin `D3` | Audible alert on alcohol detection or emergency alarm |
| **16x2 I2C LCD SDA** | Pin `A4` | I2C Data line |
| **16x2 I2C LCD SCL** | Pin `A5` | I2C Clock line |
| **USB Serial Port** | USB Cable | Connects to Laptop for 9600 Baud JSON Telemetry & Remote Killswitch |

---

## 🚀 How to Run the System

### Option A: 1-Click Launcher (Windows)
Double-click **`Launch_Cockpit.cmd`** in the project root. It will:
1. Boot the Node.js backend server and serial gateway at **`http://localhost:8080`**.
2. Automatically scan all COM ports and link to your plugged-in Arduino Receiver.
3. Open the **AegisHelm Live Cockpit** in your default web browser.

### Option B: Manual Command Line
```powershell
cd server
npm start
```
Then open `http://localhost:8080/` in your web browser.

---

## 🔗 How the Hardware Connects to the Web Dashboard

1. **Helmet Processing**:
   * Helmet Arduino reads both IR sensors (Head on `D2` + Strap on `D3`) and MQ-3 breath alcohol on `A0`.
   * Sends RF state packets (`"SAFE"`, `"ALCOHOL"`, `"NO_HELMET"`) via 433MHz RF on Pin `12`.

2. **Bike Receiver & Ignition Interlock**:
   * Bike Arduino receives the RF packet on Pin `11`.
   * If `SAFE` (Head worn + Strap fastened + No alcohol), it pulls Relay Pin `D8` LOW (CLOSED), lighting the Green LED and starting the engine.
   * If `ALCOHOL` or `NO_HELMET`, the relay stays OPEN (OFF) and the LCD displays real-time diagnostic alerts.

3. **Dual-Mode Node.js Gateway (`server/server.js`)**:
   * The Bike Arduino streams structured JSON at 10Hz over USB Serial at 9600 baud.
   * The Node.js gateway automatically detects the Arduino COM port and broadcasts telemetry through Socket.io WebSockets.
   * **Dual-Format Compatibility**: The backend parses both structured JSON telemetry and legacy text strings (`[SAFE]`, `[ALCOHOL]`, `[NO_HELMET]`).
   * **Virtual Simulator Fallback**: If hardware is unplugged, a 10Hz virtual physics simulator keeps the dashboard fully active for testing and demonstration.

4. **Two-Way Remote Killswitch**:
   * Clicking **Remote Killswitch** on the web cockpit dispatches a `LOCK` command through the serial bridge to the Arduino, immediately de-energizing Relay Pin `D8` and halting the motorcycle. Clicking it again dispatches `UNLOCK` to re-arm the ignition.

5. **Zero-Hardware Device GPS Engine**:
   * Utilizes the browser's high-accuracy `navigator.geolocation` API to pull live satellite and WiFi trilateration coordinates directly from your laptop or smartphone without requiring a separate hardware GPS module.
   * Renders an interactive, real-time OpenStreetMap/Leaflet mini-map directly inside the live cockpit and dynamically generates instant Google Maps navigation links (`https://maps.google.com/?q=lat,lon`).

6. **Automated Crash Emergency & Cloud SMS Dispatch**:
   * Automatically initiates a 20-second emergency countdown upon detecting high impact (>3.2G) or vehicle fall.
   * Dispatches automated SOS alerts with live coordinates and Google Maps routing via Twilio Cloud SMS, Telegram Bot alerts, or simulated emergency webhooks. Configuration is managed in `server/.env.example`.

7. **On-Bike 16x2 I2C Character LCD Digital Twin & Auto-Bypass**:
   * Integrates an authentic, real-time 16x2 character matrix LCD digital twin (HW-61 PCF8574T · 0x27) directly on the web cockpit with dual-theme backlight toggle (Classic Blue/White & Emerald Green/Black).
   * **Hardware Fault Tolerance**: The Arduino receiver firmware ([`arduino/Receiver/Receiver.ino`](file:///c:/Users/anike/OneDrive/Desktop/stitch_aegishelm_smart_motorcycle_dashboard/arduino/Receiver/Receiver.ino)) automatically probes the I2C bus on boot. If the physical LCD is disconnected, damaged, or unpowered, it bypasses physical LCD writes without freezing the microcontroller, streaming the exact 16-character status messages directly to the web dashboard.

---

## 🛠️ Diagnostic Sketches & Tools

* **I2C Address Scanner**: [`arduino/diagnostics/i2c_scanner/i2c_scanner.ino`](arduino/diagnostics/i2c_scanner/i2c_scanner.ino) — Identifies whether your 16x2 LCD is on `0x27` (PCF8574T) or `0x3F` (PCF8574AT).
* **LCD Display Test**: [`arduino/diagnostics/test_lcd/test_lcd.ino`](arduino/diagnostics/test_lcd/test_lcd.ino) — Tests display contrast and backlight cycling.
* **PowerShell Serial Monitor**: [`arduino/tools/monitor.ps1`](arduino/tools/monitor.ps1) — Standalone command-line monitor to inspect raw serial streams on any COM port. Run via:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\arduino\tools\monitor.ps1 -portName COM4
  ```

