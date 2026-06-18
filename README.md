# 🎛️ RCSIM Firmware Subsystem (Tiers 1, 2, 6 & Lights)

This project contains the firmware subsystem for the **RCSIM (RC Simulator / Ground Control Station)** platform. The software supports various tiers of hardware integration – ranging from simple signal converters to advanced wireless video/control hubs and hardware watchdog systems supervising single-board computers (such as the Raspberry Pi 5).

---

## 📐 System Architecture

The diagram below illustrates the signal and control flow depending on the active firmware tier:

```
                  +---------------------------+
                  |    Ground Control GCS     |
                  |          (PC App)         |
                  +-------------+-------------+
                                |
         +----------------------+----------------------+
         | (USB Serial)                                | (WiFi UDP / Video Stream)
         v                                             v
+--------+------------------+                 +--------+------------------+
|      [Tier 1 Bridge]      |                 |    [Tier 2 Wireless Hub] |
|   Arduino Serial-to-PPM  |                 |      ESP32 Controller      |
+--------+------------------+                 +--------+------------------+
         |                                             |
         | (PPM Signal)                                | (I2C Bus)
         v                                             v
+--------+------------------+                 +--------+------------------+
|       RC Transmitter      |                 |    PCA9685 PWM Driver     |
|   (e.g., RadioMaster MT12)|                 |   (Servos, ESC, Lights)   |
+--------+------------------+                 +--------+---------+--------+
         |                                                       
         | (PPM / RC Signal)                                    
         |                                                       




+--------+-------------------------------------------------------+--------+
|                      [Tier 6 Watchdog & MUX]                            |
|             ESP32 Safety Supervisor (RCSIM HAT)                         |
+------------------------------------+------------------------------------+
                                     |
                           (Heartbeat) | (Override Status)
                                     v
                  +------------------+------------------------+
                  |             Raspberry Pi 5                |
                  |        (Autonomous / SLAM Unit)           |
                  +-------------------------------------------+
```

---

## 🗂️ Module Descriptions (Tiers)

### 📟 Tier 1: Serial to PPM Converter (Arduino)
*Location:* `[Arduino Tier 1](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/Arduino%20Tier%201)`

A communication bridge converting serial text commands from the GCS into a standard PPM (Pulse Position Modulation) signal.
*   **Communication:** USB Serial (`115200 bps`), data format: `ch1,ch2,...,ch8\n`.
*   **Physical Failsafe (Signal Kill):** If no valid control frames arrive for over `500 ms`, the system disables Timer1 interrupts and switches the PPM pin (`D10`) to high-impedance mode (`INPUT`). The RC radio immediately detects this loss of signal (Trainer Lost) and triggers its own failsafe procedure.
*   **Hardware E-STOP:** Supports instant command overrides: `ESTOP` cuts off the PPM output signal and locks control, while `ARM` restores functionality.
*   **Status Signalling:** The built-in LED flashes at ~5 Hz during normal operation. In Failsafe or E-STOP state, the LED turns off.

---

### 🌐 Tier 2: Wireless Control Hub (ESP32)
*Location:* `[ESP32 Tier 2](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/ESP32%20Tier%202)`

A feature-rich wireless hub for video streaming, telemetry, and RC servo control over a network.
*   **PCA9685 Hardware Autocalibration (`calibratePCA9685`):** Utilizes a feedback loop to correct the internal oscillator frequency of the PCA9685. A test 1500 us pulse from PCA9685 channel 15 is routed directly to ESP32 GPIO 12. The ESP32 measures the pulse width using `pulseIn()` and adjusts the PCA9685 oscillator frequency until the error drops below 3 us.
*   **Video Streaming:** MJPEG video stream served over HTTP on port `81`. It runs asynchronously on FreeRTOS Core 0.
*   **UDP Control:** Receives text-based PWM commands on port `12345`.
*   **IMU Telemetry:** Reads data from an MPU6050 (gyroscope + accelerometer) and sends it as JSON over UDP to port `12347`.
*   **Failsafe Protections:** Network watchdog (automatic reconnection to AP on WiFi drop) and Control watchdog (forces all 16 channels to neutral 1500 us if UDP control data stops for >500 ms).

---

### 💡 Arduino Lights: Intelligent RC Light Controller
*Location:* `[Arduino Lights](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/Arduino%20Lights)`

A smart lighting controller decoding receiver PWM signals to control vehicle LEDs (headlights, brakes, reverse, turn signals).
*   **Full Mode:** Reacts dynamically to steering (CH1 - turn signals with a 300 ms hysteresis delay), throttle (CH2 - brake lights when stopping/slowing down, automatic reverse lights), and auxiliary switch (CH3 - toggles headlights and roof lights).
*   **Single-Channel Mode (Muxed Fallback):** Automatic fallback when only CH3 is connected. Allows toggling basic light modes using a single RC channel.
*   **PCINT (Pin Change Interrupts):** Non-blocking measurement of PWM inputs on pins D8, D9, and D10. Eliminates slow, blocking `pulseIn()` calls.
*   **Active-Low Protection:** LEDs are switched off by transitioning the Arduino pins to `INPUT` (high impedance) instead of setting them `HIGH`. This prevents reverse current flows from damaging the ESC or receiver.
*   **Failsafe:** Loss of signal for >500 ms automatically activates hazard lights (dual turn signals flashing).

---

### 🛡️ Tier 6: Hardware Watchdog & SBUS Muxer (ESP32)
*Location:* `[ESP32 Tier 6 Watchdog for RPi](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/ESP32%20Tier%206%20Watchdog%20for%20RPi)`

A safety coprocessor (supervisor) supervising the main single-board computer (Raspberry Pi 5).
*   **Heartbeat Monitor:** Watches for a heartbeat signal from the RPi 5 on pin `PIN_HEARTBEAT` (GPIO 4). A loss of heartbeat for >1000 ms indicates a system crash and triggers the failsafe state.
*   **Manual Override (Native SBUS):** Decodes SBUS signals from the RC receiver on `PIN_SBUS_RX` (GPIO 15) using UART2. Leverages the ESP32's built-in hardware UART signal inversion, eliminating the need for an external transistor inverter.
*   **I2C Bus Protection:** The internal state machine (`SystemState`) ensures safety override and neutral commands are sent to the PCA9685 exactly once during state transitions, preventing bus collisions on the shared I2C bus with the Pi.
*   **Muxer Status Feedback (`PIN_OVR_STATUS`):** Pin GPIO 5 signals the active controller state back to the Raspberry Pi (`LOW` for manual override/failsafe, `HIGH` for normal Pi operation).

---

## ⚡ Power & Safety Warnings

> [!IMPORTANT]
> **ESP32 Power Supply:** ESP32-CAM and ESP32-Wrover modules draw high peak currents (up to 500mA) during WiFi transmission and camera boot. They must be powered using a dedicated 5V BEC rated for at least 1.5A–2A.
> **Servo Power Isolation:** Servos connected to the PCA9685 must be powered from an external battery/BEC connected to the blue screw terminal (V+). Do not power servos directly from the ESP32's 5V line, as this will cause voltage drops and trigger microcontroller brownout resets.

---

## 🔧 Installation & Getting Started

1.  **Select a Tier:** Navigate to the subdirectory matching your hardware configuration.
2.  **Compilation:** Open the selected project inside Arduino IDE or VS Code with the PlatformIO extension.
3.  **Flash Firmware:** Ensure required libraries are installed (`PPMEncoder`, `Bolder Flight Systems SBUS`, `Adafruit PWM Servo Driver`). Flash the code to the microcontroller.
4.  **Autocalibration Setup (Tier 2):** Connect PCA9685 channel 15 to ESP32 GPIO 12 before powering on the board to run the calibration cycle.

---

## 📄 License

This project is licensed under the **MIT License**. For details, see the [LICENSE](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/LICENSE) file.
