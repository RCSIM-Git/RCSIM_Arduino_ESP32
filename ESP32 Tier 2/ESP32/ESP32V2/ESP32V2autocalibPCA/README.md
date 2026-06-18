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
