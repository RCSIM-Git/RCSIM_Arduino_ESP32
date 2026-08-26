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

## 🛠️ Environment Setup & Arduino IDE Configuration

### 1. Adding ESP32 Board Support
1. In Arduino IDE, open **File** -> **Preferences** (`Ctrl + ,`).
2. Paste the official Espressif package URL into **Additional Boards Manager URLs**:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Boards Manager** (**Tools** -> **Board** -> **Boards Manager...** or click the board icon on the sidebar).
4. Search for `esp32` (by *Espressif Systems*) and click **Install** (recommended version 2.0.x / 3.x).

---

### 2. Required Libraries (Library Manager)
Open the **Library Manager** (**Tools** -> **Manage Libraries...** / `Ctrl + Shift + I`) and install:

- **Adafruit PWM Servo Driver Library** (by *Adafruit*) — I2C PWM driver for PCA9685.
- **Adafruit BusIO** (by *Adafruit*) — required dependency for Adafruit sensor/driver libraries.
- **MPU6050_light** (by *rfetick*) — lightweight and fast IMU accelerometer/gyroscope driver.
- *Note:* `esp_camera`, `WiFi`, `WiFiUdp`, and `Wire` are built into the ESP32 Arduino Core and do not require external installation.
- *Optional (for related Web/PPM features):* `AsyncTCP`, `ESPAsyncWebServer`, `PPMEncoder`.

---

### 3. Compilation & Flashing Settings (Tools Menu)
To ensure reliable compilation and upload without memory overflow or flashing errors, configure the **Tools** menu as follows:

| Setting in Tools Menu | Recommended Value | Notes / Rationale |
|---|---|---|
| **Board** | `"ESP32 Wrover Module"` or `"AI Thinker ESP32-CAM"` | Select based on your board (Wrover for modules with PSRAM like Freenove/WROVER) |
| **Partition Scheme** | **`"Huge APP (3MB No OTA/1MB SPIFFS)"`** | **CRITICAL!** Default partition is too small for Wi-Fi stack + camera + MJPEG |
| **Flash Frequency** | `40MHz` | Stable SPI Flash operating frequency |
| **Flash Mode** | `DIO` | Standard, reliable Flash access mode |
| **Core Debug Level** | `None` | Eliminates serial logging overhead |
| **Erase All Flash Before Sketch Upload** | `Disabled` | Standard flashing procedure |
| **Upload Speed** | `115200` (or `921600`) | `115200` prevents CRC/timeout errors with budget USB-UART bridges |
| **Port** | Select active COM port (e.g. `COM10`) | Serial port connected to your ESP32 / programmer |

---

### 4. Firmware Flashing Procedure (ESP32-CAM Gotchas)
For standalone **AI-Thinker ESP32-CAM** boards lacking an onboard USB-UART chip:
1. **Enter Bootloader Mode:** Jumper pin **`GPIO 0` (IO0)** directly to **`GND`**.
2. Press the **`RST` (Reset)** button on the ESP32-CAM board (or power cycle the module).
3. In Arduino IDE, click **Upload** (`Ctrl + U`).
4. **Run Firmware:** Once upload finishes ("Done uploading"), **disconnect `GPIO 0` from `GND`**, and press **`RST`** to boot the firmware.
5. **Power Supply Note:** ESP32 with camera and Wi-Fi transmission draws peak currents up to 500mA. Always supply stable 5V power (e.g., from an external 5V/2A BEC), rather than weak 3.3V lines from budget USB bridges.

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
