# Smart Helmet & Bike Ignition System

A dual-Arduino safety and accident prevention system for two-wheelers. The bike ignition cannot start unless the rider is wearing the helmet, the helmet strap is fastened, and no alcohol is detected.

---

## System Architecture

```mermaid
graph TD
    subgraph Helmet_Transmitter [Arduino 1: Helmet Unit (COM7)]
        IR1[Head IR Sensor - D2]
        IR2[Strap IR Sensor - D3]
        MQ3[MQ-3 Alcohol Sensor - A0]
        RF_TX[433MHz RF Transmitter - D12]
        BUZZ1[Buzzer - D8]
        LED1[Red Indicator LED - D9]
    end

    subgraph Bike_Receiver [Arduino 2: Bike Unit (COM4)]
        RF_RX[433MHz RF Receiver - D11]
        RELAY[Engine Ignition Relay - D8]
        LED2[Green Indicator LED - D9]
        BUZZ2[Alarm Buzzer - D3]
        LCD[16x2 I2C LCD - A4/SDA, A5/SCL]
    end

    Helmet_Transmitter -- 433MHz RF (RadioHead ASK 2000bps) --> Bike_Receiver
```

---

## Hardware Pinout

### 1. Helmet Unit (Transmitter - Arduino Uno)
| Component | Arduino Uno Pin | Details |
|---|---|---|
| **Head IR Sensor** | `D2` | Active LOW (with internal pullup) |
| **Strap IR Sensor** | `D3` | Active LOW (with internal pullup) |
| **MQ-3 Alcohol Sensor** | `A0` | Analog Input (Threshold: 450) |
| **Buzzer** | `D8` | Digital Output |
| **Red Status LED** | `D9` | Digital Output |
| **433MHz FS1000A Transmitter** | `D12` | DATA Pin (RH_ASK 2000 bps) |

### 2. Bike Unit (Receiver - Arduino Uno)
| Component | Arduino Uno Pin | Details |
|---|---|---|
| **433MHz RF Receiver** | `D11` | DATA Pin (RH_ASK 2000 bps) |
| **Engine Ignition Relay** | `D8` | Active LOW Relay Module |
| **Green Status LED** | `D9` | Digital Output (Ignition Indicator) |
| **Alarm Buzzer** | `D3` | Digital Output |
| **16x2 I2C LCD (HW-61 PCF8574T)** | `A4 (SDA)`, `A5 (SCL)` | Default Address: `0x27` (5V Power) |

---

## Operating States

| Condition | Transmitter Action | Receiver Action | Engine Relay | LCD Display |
|---|---|---|---|---|
| **Both IR Active + Clean Air** | Sends `"SAFE"` | Green LED ON | **CLOSED (ON)** | `HELMET: SAFE OK`<br>`ENGINE: STARTED` |
| **Alcohol Detected (>450)** | Sends `"ALCOHOL"` | Alarm Buzzer Beeps | **OPEN (OFF)** | `ALCOHOL DETECTED`<br>`ENGINE: LOCKED!` |
| **Helmet Removed / Unbuckled** | Sends `"NO_HELMET"` | Red alert | **OPEN (OFF)** | `WEAR HELMET &`<br>`FASTEN STRAP!` |
| **Signal Timeout (>3s)** | - | Fail-safe trip | **OPEN (OFF)** | `SIGNAL LOST`<br>`ENGINE STOPPED` |

---

## Libraries Required
* [RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/) (`RH_ASK`)
* [LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Wire](https://www.arduino.cc/en/reference/wire)
* [SPI](https://www.arduino.cc/en/reference/SPI)
