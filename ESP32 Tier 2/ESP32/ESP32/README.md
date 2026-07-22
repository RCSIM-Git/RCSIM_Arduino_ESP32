# RCSIM - ESP32 Control Hub V3.1 (WiFi Lite & Camera Edition)

## 📌 Project Overview

The `ESP32.ino` sketch transforms an **ESP32 / ESP32-CAM** board into a lightweight, stable control and telemetry hub for the **RCSIM (Radio Control Simulators)** Ground Control Station (GCS). The module receives UDP control signals from the PC, drives up to 16 servos/ESCs via an I2C-connected **PCA9685** driver, streams live MJPEG video from the onboard camera, and transmits basic IMU telemetry from an **MPU6050** sensor.

Version 3.1 supports both static IP and DHCP network configurations and includes pin mapping profiles for popular dev boards (AI-Thinker, Wrover-Dev, LilyGO T-SIMCAM S3).

---

## 🚀 Key Features

1. **PWM Servo Control (PCA9685 - 16 Channels)**:
   - Receives 16-channel UDP control packets (port `12345`) and maps microsecond pulse widths (1000 - 2000 us) to 12-bit PCA9685 register values (205 - 410 ticks @ 50 Hz).
2. **MJPEG Video Streaming (Dual-Core FreeRTOS)**:
   - Dedicated HTTP server running on port `81` (`http://<IP>:81/stream`).
   - The video processing task (`videoTask`) is pinned to **Core 0**, preventing camera frame processing from delaying servo control timing on Core 1.
3. **IMU Telemetry (MPU6050)**:
   - Transmits JSON telemetry containing accelerometer (`ax`, `ay`, `az`) and gyroscope (`gx`, `gy`, `gz`) readings to GCS port `12347` at 20 Hz (every 50 ms).
4. **Failsafe System**:
   - Automatically forces neutral PWM pulse widths (1500 us) across all 16 channels if UDP control packets cease for > 500 ms (`FAILSAFE_TIMEOUT_MS`).
5. **Hardware Board Profile Support**:
   - Out-of-the-box pin configurations for `BOARD_AI_THINKER`, `BOARD_WROVER_DEV`, and `BOARD_LILYGO_TSIMCAM_S3`.

---

## 🔌 Wiring & Pinout

### I2C Bus (PCA9685 & MPU6050)

| Board / Profile | SDA Pin | SCL Pin | Notes |
|---|---|---|---|
| **BOARD_WROVER_DEV** | `GPIO 13` | `GPIO 14` | Remapped from 21/22 to prevent SCCB camera pin collision |
| **BOARD_AI_THINKER** | `GPIO 21` | `GPIO 22` | Standard AI-Thinker pinout |
| **LILYGO_TSIMCAM_S3** | `GPIO 21` | `GPIO 46` | Dedicated S3 pin mapping |

---

## 📡 UDP Telemetry Payload (JSON)

JSON packet sent from ESP32 to the GCS PC IP on port **12347**:

```json
{
  "imu": {
    "ax": 0.02,
    "ay": -0.01,
    "az": 1.00,
    "gx": 0.10,
    "gy": -0.05,
    "gz": 0.00
  }
}
```

---

## 🛠️ Required Libraries

To compile in Arduino IDE or PlatformIO, ensure the following libraries are installed:

- `Adafruit PWMServoDriver Library`
- `MPU6050_light`
- `esp_camera` (included in ESP32 Arduino Board Support Package)

---

## ⚙️ Hardware Configuration Flags

Main configuration macros in `ESP32.ino`:

```cpp
#define ENABLE_CAMERA true    // Enable MJPEG Video Stream (port 81)
#define ENABLE_IMU    true    // Enable IMU Telemetry (port 12347)

// Select active hardware profile:
//#define BOARD_AI_THINKER
#define BOARD_WROVER_DEV
//#define LILYGO_TSIMCAM_S3
```
