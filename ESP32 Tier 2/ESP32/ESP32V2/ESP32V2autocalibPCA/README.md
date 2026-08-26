# RCSIM - Wireless Control Hub for ESP32 with PCA9685 Autocalibration (V3.2)

This project is a wireless control and telemetry hub that combines MJPEG video streaming, IMU telemetry (MPU6050), and servo steering control over a network via the PCA9685 driver with a feedback-loop hardware autocalibration system.

## Key Features

1. **PCA9685 Hardware Autocalibration (`calibratePCA9685`):**
   * PCA9685 drivers rely on an internal 25 MHz oscillator, which frequently suffers from manufacturing variations and thermal drift. This causes the actual PWM pulse duration to differ from the requested microsecond settings.
   * The hub uses a hardware feedback loop: channel 15 (`CALIBRATION_CH`) of the PCA9685 outputs a test 1500 us pulse directly to the ESP32 GPIO 12 (`CALIBRATION_PIN`).
   * The ESP32 measures the pulse width using `pulseIn()` and adjusts the PCA9685 oscillator frequency value in a loop (up to 15 iterations) until the deviation is under 3 microseconds.
   * The resulting calibrated frequency is applied using `pca.setOscillatorFrequency(currentFreq)`.

2. **UDP Wireless Control:**
   * Receives comma-separated text control values (e.g., `"1500,1500,1000,..."`) over UDP port `12345`.
   * Directly translates microsecond values (1000-2000 us) to PCA9685 register values without slow floating-point math.
   * Filters out corrupted packets (values outside the safe 800 - 2200 us range are discarded).

3. **Video Stream (ESP32-CAM):**
   * Streams MJPEG video frames over HTTP on port `81`.
   * Integrates built-in camera profiles for AI Thinker, ESP32-Wrover-Dev, and LilyGo T-SimCam S3 boards.
   * Multi-threaded execution via FreeRTOS (video streaming is isolated to Core 0 to avoid blocking servo commands).

4. **IMU Telemetry:**
   * Reads sensor data (accelerometer + gyroscope) from the MPU6050 sensor via the I2C bus.
   * Transmits real-time JSON packets over UDP to the Ground Control Station PC on port `12347`.

5. **Failsafe Watchdogs:**
   * **Network Watchdog:** Detecting a loss of WiFi connection (for >5s) initiates a network reconnection loop back to the AP.
   * **Control Watchdog:** If UDP control packets stop arriving for more than `500 ms`, the hub triggers `triggerFailsafe()`, instantly resetting all 16 PCA9685 channels to their neutral position (1500 us).

## Wiring Diagram (Required for Autocalibration)

To enable the autocalibration feedback loop, you must connect:
* **PCA9685 Channel 15 (PWM Signal - yellow/white cable)** -> **ESP32 GPIO 12**
* Shared ground (GND) must be connected between the ESP32, PCA9685, and the servo power source.

## I2C Configuration

Peripherals are connected to specific I2C pins depending on the selected hardware board profile:
* E.g., for the WROVER profile: `SDA = 13`, `SCL = 14`.
* The hub runs an I2C address scanner on startup (`i2c_scan()`), logging all detected devices over the serial terminal (115200 bps) and searching for `0x40` (PCA9685) and `0x68` (IMU).

---

## 🛠️ Environment Setup & Arduino IDE Configuration

### 1. Adding ESP32 Board Support
1. In Arduino IDE, go to **File** -> **Preferences** (`Ctrl + ,`).
2. Add the official Espressif boards manager URL into **Additional Boards Manager URLs**:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Boards Manager** (**Tools** -> **Board** -> **Boards Manager...**).
4. Search for `esp32` (by *Espressif Systems*) and click **Install** (version 2.0.x / 3.x).

---

### 2. Required Libraries (Library Manager)
In **Library Manager** (**Tools** -> **Manage Libraries...** / `Ctrl + Shift + I`), install:

- **Adafruit PWM Servo Driver Library** (by *Adafruit*) — I2C servo driver for PCA9685.
- **Adafruit BusIO** (by *Adafruit*) — dependency for Adafruit I2C/SPI devices.
- **MPU6050_light** (by *rfetick*) — lightweight IMU accelerometer/gyroscope driver.
- *Note:* `esp_camera`, `WiFi`, `WiFiUdp`, and `Wire` are built into the ESP32 Arduino Core.

---

### 3. Compilation & Flashing Settings (Tools Menu)

| Setting in Tools Menu | Recommended Value | Description |
|---|---|---|
| **Board** | `"ESP32 Wrover Module"` or `"AI Thinker ESP32-CAM"` | Choose according to your hardware board |
| **Partition Scheme** | **`"Huge APP (3MB No OTA/1MB SPIFFS)"`** | **CRITICAL!** Required to fit the Wi-Fi and MJPEG video stack |
| **Flash Frequency** | `40MHz` | Stable SPI Flash frequency |
| **Flash Mode** | `DIO` | Standard Flash mode |
| **Core Debug Level** | `None` | Disables serial debug output overhead |
| **Erase All Flash Before Sketch Upload** | `Disabled` | Standard upload mode |
| **Upload Speed** | `115200` (or `921600`) | `115200` guarantees reliable flashing across various USB adapters |
| **Port** | Select active COM port (e.g. `COM10`) | Serial programmer COM port |

---

### 4. Flashing Instructions (ESP32-CAM)
1. **Enter Bootloader Mode:** Connect pin **`GPIO 0` (IO0)** to **`GND`**.
2. Press the **`RST`** button on the board.
3. In Arduino IDE, click **Upload** (`Ctrl + U`).
4. Once completed: **disconnect `GPIO 0` from `GND`** and press **`RST`** to run the firmware.
5. **Power:** Always power the ESP32 module with a reliable 5V power supply (e.g. external 5V/2A BEC), as the camera and Wi-Fi pull high peak currents.

