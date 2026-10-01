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

## 🛒 Bill of Materials (BOM — Delivered Hardware)

Verified components delivered for the **ARRMA Mojave 4S / 5G** project:

### 1. RC Transmitter & Receivers:
| # | Component | Model / Spec | Qty | Function / Notes |
|---|---|---|:---:|---|
| 01 | **Pistol Transmitter** | **RadioMaster MT12 ELRS 2.4 GHz** | 1 set | Driver transmitter (EdgeTX + internal ExpressLRS module) |
| 02 | **Model Receivers** | **RadioMaster ER3C-i** and **ER5C-i** | 1 ea. | CRSF input (420,000 bps) & telemetry uplink to MT12 |

### 2. Main Flight Controller & Grove I2C Ecosystem:
| # | Component | Model / Spec | Qty | Function / Notes |
|---|---|---|:---:|---|
| 03 | **Microcontroller** | **ESP32-S3 DevKit (N8R8 / N16R8 Waveshare)** | 1 | Dual-Link Muxer, FreeRTOS Dual-Core, Wi-Fi 5G + UART |
| 04 | **Terminal Shield** | **44-pin Screw Terminal (ARK) Shield** | 1 | Vibration-proof, solderless wiring breakout |
| 05 | **I2C Multiplexer** | **Grove Hub I2C TCA9548A (8 Ports)** | 1 | I2C bus expansion and device isolation (0x70) |
| 06 | **Servo/ESC Driver** | **Grove PCA9685 (16-channel 12-bit PWM)** | 1 | Steering servo & Spektrum Firma ESC control (0x40) |
| 07 | **IMU Sensor 6-DoF** | **Grove LSM6DS3 (Accel + Gyro)** | 1 | Attitude telemetry (Pitch/Roll/Yaw) & G-forces (0x6A) |
| 08 | **GPS Module** | **Grove GPS Air530 with Ceramic Antenna** | 1 | Position, speed, heading over NMEA UART (9600 bps) |
| 09 | **Terminal Adapter** | **Grove 4-pin to Screw Terminal** | 1 | Solderless external wiring interface |

### 3. Power, Cabling & Protection:
| # | Component | Model / Spec | Qty | Function / Notes |
|---|---|---|:---:|---|
| 10 | **Power Bank** | **everActive EB-22QB 20,000 mAh (20W PD/QC)** | 1 | Dedicated power for ESP32-S3 and 5G smartphone |
| 11 | **USB-C Cables** | 1x 90° angled 30 cm (phone) + 1x straight 30 cm (ESP) | 2 | Clean cable routing inside the cockpit |
| 12 | **Signal Wiring** | Grove 4-pin cables + Grove-to-Dupont + Dupont F-F | 1 set | I2C bus, UART, and CRSF connections |
| 13 | **JR Extensions** | 30 cm 22AWG flat MSP cables | 2 | Steering servo and ESC connection to PCA9685 |
| 14 | **Enclosure** | **Pawbol S-Box 216C IP65 (120×80×50 mm)** | 1 | Dust and water-resistant protection |
| 15 | **Mounting Hardware** | PG7 IP68 cable glands + **3M Dual Lock SJ3550** | 1 set | Sealed cable passthroughs & vibration damping mount |
| 16 | **5G FPV Rig** | SmallRig 2164 clamp + phone mount + Peltier cooler | 1 set | Transmitter phone mount with active cooling |

---

## 🔌 Hardware Pinout (ESP32-S3 44-pin Terminal Shield)

| Peripheral / Module | Shield Terminal | ESP32-S3 Pin | Signal / Protocol | Notes |
|---|---|---|---|---|
| **Grove TCA9548A Hub** | `SDA` | `GPIO 8` | I2C Data | Main I2C data bus (TCA9548A, PCA9685, LSM6DS3) |
| **Grove TCA9548A Hub** | `SCL` | `GPIO 9` | I2C Clock | I2C Fast Mode clock (400 kHz) |
| **Grove TCA9548A Hub** | `3V3` / `5V` | `3.3V` / `5V` | VCC | Module logic power |
| **Common Ground** | `GND` | `GND` | Ground | **Essential common ground** (ESP, PCA, ESC, BEC) |
| **Grove Air530 GPS** | `18` | `GPIO 18` (RX1) | UART RX <- GPS TX | NMEA sentence parsing (9600 bps) |
| **Grove Air530 GPS** | `17` | `GPIO 17` (TX1) | UART TX -> GPS RX | Module configuration (optional) |
| **ER3C-i / ER5C-i Receiver** | `15` | `GPIO 15` (RX2) | CRSF RX <- Recv TX | CRSF control stream from MT12 (420,000 bps) |
| **ER3C-i / ER5C-i Receiver** | `16` | `GPIO 16` (TX2) | CRSF TX -> Recv RX | Return telemetry uplink to MT12 display |
| **Battery Divider (VBAT)** | `1` | `GPIO 1` (ADC1) | Analog IN (0-3.3V) | 4S LiPo monitoring (R1=10k, R2=2.2k) |
| **ESP32 Power** | `USB-C` | USB-C Port | 5V DC (PD) | Powered from everActive EB-22QB power bank |

> [!CAUTION]
> **ESP32-S3 Octal Flash/PSRAM Pin Safety:**
> Pins `GPIO 33, 34, 35, 36, 37` are reserved for internal Octal SPI memory on N8R8 and N16R8 boards. **Never connect external signals to these pins!** Battery sensing is routed to safe `ADC1_CH0` (`GPIO 1`).

---

## ⚙️ RadioMaster ER3C-i / ER5C-i Setup (ExpressLRS)

1. Connect to receiver Wi-Fi access point (default password: `expresslrs`).
2. Navigate to `http://10.0.0.1` in your browser.
3. Under **Model / Hardware**:
   - Set **Serial Protocol** to `CRSF`.
   - Set TX Pin to connect to ESP32 `GPIO 15` (RX2).
   - Set RX Pin to connect to ESP32 `GPIO 16` (TX2).
   - Baud rate: `420000` (or `115200`).
4. On the RadioMaster MT12 (EdgeTX), run *Model Setup -> Telemetry -> Discover new sensors*.
   - Discovered sensors: `RxBt`, `GPS`, `Ptch`, `Roll`, `Yaw`, and `RQly`.

---

## 🎮 Channel Mapping

| Channel | Name | Function | Signal Range | Safe State |
|---|---|---|---|---|
| **CH 1** | Steering | Steering Servo (PCA9685 CH0) | 1000 µs – 1500 µs – 2000 µs | `1500 µs` (Center) |
| **CH 2** | Throttle | ESC Throttle/Brake (PCA9685 CH1) | 1000 µs – 1500 µs – 2000 µs | `1500 µs` (Neutral) |
| **CH 5** | AUX 1 | Arming Switch (ARM) | > 1350 µs = ARMED, < 1350 µs = STOP | `1000 µs` (Disarmed) |
| **CH 6** | AUX 2 | Muxer Switch (Radio / 5G) | < 1300 µs: RF, 1300-1700: AUTO, > 1700 µs: 5G | AUTO (5G fallback) |

---

## 🛠️ Required Libraries

- **Adafruit PWM Servo Driver Library** (for PCA9685)
- **TinyGPSPlus** (for Grove Air530 NMEA parsing)
*(Grove LSM6DS3 IMU driver and Grove TCA9548A hub manager are built directly into the firmware with zero external dependencies!)*
