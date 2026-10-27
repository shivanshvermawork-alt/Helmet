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

### 1. Transmitter Unit (Mounted on Helmet — Arduino Uno / Nano · COM7)
| Component | Arduino Pin | Notes / Calibration |
| :--- | :--- | :--- |
| **Helmet Presence IR (Head)** | Pin `D2` | Active LOW (Detects head inside helmet via internal pullup) |
| **Eye Blink / Drowsiness Sensor** | Pin `D3` | Active LOW (Normal blinks allowed; >1.5s prolonged eye closure triggers Drowsiness Alert) |
| **Alcohol Sensor (MQ-3 AO)** | Pin `A0` | Analog breath measurement (Clean air ~250, Sober Threshold: **650**) |
| **Local Helmet Buzzer (+)** | Pin `D8` | Acoustic alert on alcohol detection, drowsiness, or crash impact |
| **Red Status Indicator LED** | Pin `D9` | Solid ON on alcohol alert; pulses on drowsiness; flashes on crash |
| **433MHz RF Transmitter DATA** | Pin `D12` | 2000 bps ASK modulation (`RH_ASK` driver on FS1000A) |
| **MPU-6050 6-DOF IMU (SDA / SCL)** | Pins `A4` / `A5` | Auto-detected for live 3-axis G-force, pitch (nod), roll (lean), and crash detection |

### 2. Receiver Unit (Mounted on Motorcycle — Arduino Uno · COM4)
| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **433MHz RF Receiver DATA** | Pin `D2` | Receives RF packets from helmet transmitter (XY-MK-5V receiver) |
| **Starter Ignition Relay IN** | Pin `D8` | Controls bike ignition circuit (Active LOW: LOW = ON, HIGH = OFF) |
| **Green Ignition Status LED** | Pin `D9` | Indicates engine ignition circuit is armed and live |
| **Vehicle Warning Buzzer (+)** | Pin `D3` | Audible alert on alcohol detection, drowsiness, or emergency cutoff |
| **16x2 I2C LCD SDA** | Pin `A4` | I2C Data line (PCF8574T @ `0x27`) |
| **16x2 I2C LCD SCL** | Pin `A5` | I2C Clock line |
| **USB Serial Port** | USB Cable | 9600 Baud live JSON & telemetry bridge to PC/Web Gateway |

---

## 🚀 How to Run the System

### Option A: 1-Click Localhost Launcher (Recommended)
Double-click **`Start_Localhost.cmd`** in the project root. It will:
1. Boot the Python hardware gateway server at **`http://localhost:8000`**.
2. Automatically connect to COM4 at 9600 baud.
3. Stream Server-Sent Events (`/events`) with live MPU 3-axis telemetry and sensor states.
4. Open the **AegisHelm Live Cockpit** in your default web browser.

### Option B: Node.js Launcher
Double-click **`Launch_Cockpit.cmd`** to run via Node.js Express & Socket.io on `http://localhost:8080/`.

---

## 🔗 How the Hardware Connects to the Web Dashboard

1. **Helmet Processing ([Blink/Blink.ino](Blink/Blink.ino))**:
   * Helmet Arduino reads Head IR on `D2`, Eye Blink on `D3`, MQ-3 breath alcohol on `A0`, and MPU-6050 on `A4/A5`.
   * Broadcasts wireless RF telemetry packets containing current state + 3-axis coordinates:
     `STATE:ax,ay,az,pitch,roll,gForce` (e.g. `SAFE:0.02,0.01,0.99,0.0,0.0,1.00`).

2. **Bike Receiver & Ignition Interlock ([Receiver/Receiver.ino](Receiver/Receiver.ino))**:
   * Receives 433MHz RF telemetry on Pin `D2`.
   * **Safe State**: Pulls Relay Pin `D8` LOW (CLOSED), lighting the Green LED and starting the engine.
   * **Drowsiness State**: Triggers buzzer alert on `D3` and handlebar LCD display (`DROWSINESS ALERT / TAKE A BREAK!`).
   * **Alcohol State**: De-energizes relay (OFF), locks engine, sounds alarm, and displays `ALCOHOL DETECTED / ENGINE: LOCKED!`.
   * **No Helmet**: De-energizes relay and displays `WEAR HELMET & / FASTEN STRAP!`.
   * **Crash Impact**: High G-force spike (>2.6G) cuts ignition immediately and broadcasts SOS coordinates.

3. **Live Python Localhost Gateway ([cockpit_server.py](cockpit_server.py))**:
   * Reads COM4 serial stream at 9600 baud, decodes 3-axis acceleration and angles, and broadcasts via Server-Sent Events (`/events`).
   * Features built-in port release endpoint (`/api/release-port`) to allow firmware uploading without server interruption.

4. **Widescreen Live Cockpit Dashboard ([code.html](aegishelm_live_cockpit_dashboard/code.html))**:
   * **Digital Twin Handlebar LCD**: Real-time mirroring of the motorcycle 16x2 screen.
   * **3-Axis & G-Force Gauges**: Live readout of X-Axis (lateral tilt), Y-Axis (surge), Z-Axis (heave), and dynamic G-Force.
   * **Interactive 3D Attitude Manipulation Bus**: Live pitch and roll angles dynamically orient the 3D helmet model.
   * **Eye Blink / Microsleep Monitor**: Displays live PERCLOS index and drowsiness alert status.
   * **Two-Way Remote Killswitch**: Directly cuts or re-arms the ignition relay from the web UI.

---

## 🛠️ Diagnostic Sketches & Tools

* **I2C Address Scanner**: [`arduino/diagnostics/i2c_scanner/i2c_scanner.ino`](arduino/diagnostics/i2c_scanner/i2c_scanner.ino) — Identifies whether your 16x2 LCD is on `0x27` (PCF8574T) or `0x3F` (PCF8574AT).
* **LCD Display Test**: [`arduino/diagnostics/test_lcd/test_lcd.ino`](arduino/diagnostics/test_lcd/test_lcd.ino) — Tests display contrast and backlight cycling.
* **PowerShell Serial Monitor**: [`arduino/tools/monitor.ps1`](arduino/tools/monitor.ps1) — Standalone command-line monitor to inspect raw serial streams on any COM port. Run via:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\arduino\tools\monitor.ps1 -portName COM4
  ```

