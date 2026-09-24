# RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub & Dual-Link Hybrid)

## 📌 Overview

The **Tier 2 Pro (V4)** firmware for the **ESP32** microcontroller serves as an advanced onboard vehicle controller, arbitration multiplexer, and telemetry hub for RC vehicles and rovers (such as the ARRMA Mojave 4S).

It combines native **CRSF (Crossfire / ExpressLRS)** support with long-range remote driving over **5G Internet / Tailscale VPN**.

### Key Features:
1. **Dual-Link Hybrid Mode (Intelligent Muxer):**
   - **Link 1: Direct RF Link (Ultra Low-Latency):** Local transmitter (e.g., RadioMaster MT12) paired with a **RadioMaster ER5C V2** receiver connected via hardware **UART (420,000 baud)**.
   - **Link 2: Long-Range 5G Internet Link:** ESP32 connects to an onboard mobile Wi-Fi hotspot (e.g. Redmi 15 5G), receiving control packets from RCSIM GCS on PC over Tailscale VPN / UDP.
   - **Automatic Arbiter:** Local RF takes priority by default. In case of RF signal loss (>150 ms) or transmitter shutdown, the vehicle smoothly transitions to 5G Internet control.
   - **Transmitter Mode Switch (AUX2 / Channel 6):**
     - High position (< 1300 µs): Force local RF only.
     - Middle position (1300–1700 µs): AUTO Muxer mode (RF priority with 5G fallback).
     - Low position (> 1700 µs): Force 5G Internet mode.
2. **Bidirectional CRSF Telemetry:**
   - Onboard sensors generate standard CRSF telemetry frames (`0x1E Attitude`, `0x08 Battery`, `0x02 GPS`, `0x14 Link Statistics`).
   - Packets are dispatched **simultaneously**:
     - Back to the ER5C V2 receiver over UART (displays battery voltage, GPS coords, and artificial horizon directly on the RadioMaster MT12 screen).
     - Over 5G VPN / UDP to RCSIM PC GCS (for the PC HUD, dashboard, and Force Feedback).
3. **PCA9685 I2C Servo Controller:**
   - Directly drives steering servos and ESCs (Spektrum Firma / Hobbywing) via I2C Fast Mode (400 kHz) with automatic bus recovery.
4. **Hardware Fail-Safe & Non-blocking Startup:**
   - Hard fail-safe triggers neutral (`1500 µs`) if both links timeout (>150 ms) or if disarmed (Channel 5 / AUX1 < 1350 µs).
   - Non-blocking Wi-Fi startup: the RF link functions from millisecond 1 after boot, even if the phone's hotspot is off or connecting in the background.

---

## 🛒 Bill of Materials (BOM)

Complete list of components required to build the dual-link hybrid (RF + 5G/VPN) control system:

### 1. Onboard Vehicle Electronics:
| # | Component | Model / Spec | Qty | Function / Notes |
|---|---|---|:---:|---|
| 1 | **Microcontroller** | ESP32 DevKit V1 / WROOM-32 / WROVER / ESP32-S3 | 1 | Main onboard controller, FreeRTOS Dual-Core, Muxer |
| 2 | **RF Receiver (ELRS)** | RadioMaster ER5C V2 (or ER4 / ER6 / RP1) | 1 | CRSF UART input (420k bd) & telemetry uplink to MT12 |
| 3 | **Servo/ESC Driver** | PCA9685 (I2C 16-channel 12-bit PWM) | 1 | High-precision steering servo & ESC control |
| 4 | **Power Supply (BEC)** | Step-Down Regulator / BEC 5V (min. 3A) | 1 | Clean 5V power for ESP32, PCA9685, and ER5C receiver |
| 5 | **IMU Sensor** | MPU6050 (GY-521 / MPU9250) | 1 | Attitude telemetry (Pitch, Roll, Yaw) for artificial horizon |
| 6 | **GPS Module** | Beitian BN-220 / BN-180 (or u-blox M8N) | 1 | Geo-coordinates, true ground speed, heading, and sats |
| 7 | **Battery Divider** | Resistors: 10 kΩ (R1) + 2.2 kΩ (R2) 1% 0.25W | 1 set | Pack voltage measurement on ADC1 (GPIO 33) |
| 8 | **Buffer Capacitor** | Electrolytic 470µF – 1000µF / 10V-16V Low-ESR | 1 | 5V rail filtering against servo brownout spikes |
| 9 | **Wiring** | DuPont jumper wires (F-F / M-F) | 1 set | Signal connections for I2C, UART, power, and ground |

### 2. Onboard Networking / FPV Video:
| # | Component | Model / Spec | Qty | Function / Notes |
|---|---|---|:---:|---|
| 10 | **Onboard Smartphone** | Redmi 15 5G (or any Android 5G smartphone) | 1 | Wi-Fi hotspot for ESP32, WebRTC FPV camera, 5G Tailscale |
| 11 | **Phone Mount** | 3D Printed / rigid chassis bracket | 1 | Secure phone mounting inside the ARRMA Mojave cockpit |

### 3. Base Station / Driver Station (Home / PC):
| # | Component | Model / Spec | Qty | Function / Notes |
|---|---|---|:---:|---|
| 12 | **Pistol Transmitter** | RadioMaster MT12 (ExpressLRS 2.4 GHz) | 1 | Local RF control or direct USB CRSF trainer link to PC |
| 13 | **PC / Laptop** | Windows 10/11 or macOS with RCSIM GCS | 1 | Ground station, FPV video display, wheel/controller, HUD |
| 14 | **VPN Software** | Tailscale (free personal account) | - | Encrypted P2P tunnel between PC and onboard phone |

---

## 🔌 Hardware Pinout

| Peripheral / Module | ESP32 Pin | Module Pin | Description / Notes |
|---|---|---|---|
| **PCA9685 & IMU SDA** | `GPIO 13` | SDA | I2C Data bus (4.7k pull-up resistors recommended) |
| **PCA9685 & IMU SCL** | `GPIO 14` | SCL | I2C Clock bus (400 kHz Fast Mode) |
| **PCA Calibration** | `GPIO 12` | CH15 | Optional oscillator calibration feedback |
| **ER5C V2 Receiver TX** | `GPIO 16` (RX2) | CRSF TX | Control frame input from ELRS receiver (420,000 bps) |
| **ER5C V2 Receiver RX** | `GPIO 17` (TX2) | CRSF RX | Telemetry uplink back to MT12 transmitter |
| **GPS TX (NMEA)** | `GPIO 32` (RX1) | TXD | GPS NMEA stream (9600 bps) |
| **Battery (VBAT)** | `GPIO 33` (ADC1) | Divider | Voltage divider R1=10k, R2=2.2k (~5.545 ratio) |
| **ESP32 Power** | `5V / VIN` | BEC 5V | External 5V/2-3A BEC (common GND with ESC and servo!) |

---

## ⚙️ RadioMaster ER5C V2 Setup (ExpressLRS)

1. Access the ER5C V2 Web UI (via Wi-Fi or ExpressLRS Configurator).
2. Under **Model / Hardware**:
   - Set **Pin 1** as `CRSF TX` $\rightarrow$ connect to ESP32 `GPIO 16` (RX2).
   - Set **Pin 2** as `CRSF RX` $\rightarrow$ connect to ESP32 `GPIO 17` (TX2).
   - Baud rate: `420000`.
3. In EdgeTX on the RadioMaster MT12, run *Telemetry -> Discover new sensors* to discover `RxBt`, `GPS`, `Pitch`, `Roll`, `Yaw`, and `RQly`.

---

## 🎮 Channel Mapping

| Channel | Name | Function | Signal Range |
|---|---|---|---|
| **CH 1** | Steering | Steering Servo (PCA9685 CH0) | 1000 µs (left) – 1500 µs – 2000 µs (right) |
| **CH 2** | Throttle | ESC Throttle/Brake (PCA9685 CH1) | 1000 µs (reverse) – 1500 µs (neutral) – 2000 µs (forward) |
| **CH 5** | AUX 1 | Arming Switch (ARM) | > 1350 µs = ARMED, < 1350 µs = DISARMED (Safe stop) |
| **CH 6** | AUX 2 | Muxer Switch (RF vs 5G) | < 1300 µs = Force RF, 1300-1700 = AUTO, > 1700 µs = Force 5G |

---

## 🛠️ Required Libraries

In the Arduino IDE Library Manager:
- **Adafruit PWM Servo Driver Library**
- **Adafruit BusIO**
- **TinyGPSPlus**
- **MPU6050_light**
