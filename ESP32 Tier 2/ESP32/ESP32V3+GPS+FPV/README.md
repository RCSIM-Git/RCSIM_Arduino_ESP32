# RCSIM - ESP32 Control Hub V3.3 (Full OSD & GPS Edition)

## 📌 Project Overview

This firmware module for the **ESP32** microcontroller acts as a wireless control and telemetry hub for the **RCSIM (Radio Control Simulators)** platform. The MCU handles incoming UDP control signals, drives servo PWM channels (via a PCA9685 driver), streams FPV MJPEG video, parses NMEA GPS data, and transmits real-time telemetry to both the Ground Control Station (GCS on PC) and FPV Goggles (Walksnail MSP OSD).

The module has been engineered according to **Safety-Critical** standards, preventing control loop lockups under heavy Electromagnetic Interference (EMI) conditions common in high-power RC environments.

---

## 🚀 Key Features

1. **Universal IMU Abstraction Layer**:
   - Supports sensors: **MPU6050**, **MPU9250 / GY-91**, **BNO08x** (BNO080/085 - hardware quaternion fusion), **BMX160**.
   - Switch active sensor via a single macro: `#define ACTIVE_IMU`.
   - Integrated hardware Digital Low-Pass Filter (DLPF ~42Hz for MPU6050/MPU9250) to mitigate motor vibrations.

2. **Automatic I2C Bus Recovery & Timeout**:
   - Hard I2C timeout configured to 10 ms (`Wire.setTimeOut(10)`).
   - `checkAndRecoverI2C()` routine automatically detects bus faults and performs real-time bus recovery without resetting the ESP32.

3. **Task Watchdog Timer (Hardware WDT)**:
   - Dedicated 2-second WDT timer monitoring the main `loop()` thread on Core 1.
   - Prevents permanent system freezes and vehicle flyaways (*Flyaway / Runaway Protection*).

4. **Native Walksnail MSP OSD Integration**:
   - Transmits MultiWii Serial Protocol (MSP) frames directly to the Walksnail VTX:
     - `MSP_ATTITUDE` (Cmd 108, 20 Hz) – Artificial horizon, roll & pitch angles.
     - `MSP_ANALOG` (Cmd 110, 5 Hz) – Main battery voltage & Wi-Fi RSSI.
     - `MSP_STATUS` (Cmd 101, 5 Hz) – Arming status (ARM/DISARM based on Failsafe).
     - `MSP_RAW_GPS` (Cmd 106, 5 Hz) – GPS position, altitude, ground speed & satellite count.
     - `MSP_NAME` (Cmd 10, 0.5 Hz) – Vehicle identifier ("RCSIM V3.3").

5. **MJPEG Video Streaming (Dual Core FreeRTOS)**:
   - Dedicated `videoTask` pinned to **Core 0** (HTTP port 81: `/stream`).
   - Completely decouples camera graphics workload from the critical servo control loop on Core 1.

6. **Non-Blocking Battery Voltage Sensing (IIR / EMA Filter)**:
   - Low-pass Exponential Moving Average filter on ADC1 (GPIO 33).
   - Zero execution latency (sample time reduced from 250 us to 10-15 us), eliminating PWM control jitter.

7. **PCA9685 Feedback Loop Autocalibration**:
   - Measurement loop (`calibratePCA9685`) using feedback from PCA9685 Channel 15 to ESP32 GPIO 12 (`pulseIn`).
   - Dynamically tunes internal oscillator frequency within a 3 us tolerance window.

8. **Failsafe & Control Safety**:
   - Enforces neutral PWM pulse width (1500 us) across all 16 channels if UDP control packets cease for > 500 ms.

---

## 🔌 Wiring & Pinout (WROVER-DEV)

| Peripheral / Module | ESP32 Pin | Function / Notes |
|---|---|---|
| **I2C SDA** | `GPIO 13` | I2C Data Bus (PCA9685, IMU) |
| **I2C SCL** | `GPIO 14` | I2C Clock Bus |
| **Walksnail VTX TX** | `GPIO 2` (TX2) | Connects to Walksnail RX |
| **Walksnail VTX RX** | `GPIO 15` (RX2) | Connects to Walksnail TX |
| **GPS TX** | `GPIO 32` (RX1) | NMEA Serial In (HardwareSerial 1) |
| **VBAT Sensing** | `GPIO 33` (ADC1) | Voltage divider R1=10k, R2=2.2k (~5.545x ratio) |
| **PCA Calib IN** | `GPIO 12` | Calibration feedback from PCA9685 CH15 |
| **Camera XCLK** | `GPIO 21` | Camera System Clock |
| **Camera SIOD / SIOC**| `GPIO 26 / 27` | Camera SCCB / I2C lines |

---

## 📡 UDP Telemetry Payload (JSON)

The ESP32 broadcasts telemetry JSON packets to the GCS PC IP on port **12347** at 20 Hz:

```json
{
  "imu": {
    "ax": 0.02,
    "ay": -0.01,
    "az": 1.00,
    "gx": 0.10,
    "gy": -0.05,
    "gz": 0.00,
    "roll": 1.2,
    "pitch": -0.5,
    "yaw": 180.0
  },
  "gps": {
    "valid": true,
    "lat": 52.229675,
    "lng": 21.012230,
    "alt": 120.5,
    "speed": 15.4,
    "sats": 10
  },
  "vbat": 12.42,
  "rssi": -55
}
```

---

## 🛠️ Required Libraries

To compile in Arduino IDE or PlatformIO, ensure the following libraries are installed:

- `Adafruit PWMServoDriver Library`
- `TinyGPSPlus`
- `esp_camera` (included in ESP32 Arduino Core)
- **Depending on `ACTIVE_IMU` configuration**:
  - For `IMU_TYPE_MPU6050`: `MPU6050_light`
  - For `IMU_TYPE_MPU9250`: `MPU9250_WE`
  - For `IMU_TYPE_BNO08X`: `Adafruit BNO08x`
  - For `IMU_TYPE_BMX160`: `DFRobot_BMX160`

---

## ⚙️ Configuration Flags

Header configuration switches:

```cpp
#define ENABLE_CAMERA                true   // Enable MJPEG Video Stream
#define ENABLE_IMU                   true   // Enable IMU sensor
#define ACTIVE_IMU                   IMU_TYPE_MPU6050 // Select active IMU type
#define ENABLE_PCA_AUTOCALIBRATION   true   // Enable PCA9685 frequency autocalibration
#define ENABLE_WALKSNAIL_OSD         true   // Enable Walksnail MSP OSD frames
#define ENABLE_BATTERY               true   // Enable ADC battery sensing
#define ENABLE_GPS                   true   // Enable NMEA GPS parsing
```
