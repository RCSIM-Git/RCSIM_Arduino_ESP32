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
         +----------------------+----------------------+----------------------+
         | (USB Serial / VCP)   | (Direct USB-C CRSF)  | (WiFi UDP / Stream)  |
         v                      v                      v                      v
+--------+----------+  +--------+-----------+ +--------+----------+  +--------+----------+
|  [Tier 1 Bridge]  |  | [EdgeTX USB-VCP]   | | [Tier 2 Hub]     |  | [Tier 3 Direct]   |
| Arduino / RP2350  |  | RadioMaster MT12   | | ESP32 Controller |  | RadioMaster Nomad |
+--------+----------+  | & Pocket Radio     | +--------+----------+  +--------+----------+
         | (PPM)       +--------+-----------+          | (I2C)                | (CRSF RF)
         v                      |                      v                      v
+--------+----------+           | (ELRS RF +           +------------------+   |
| Standard RC Radio |           | Telemetry Mirror)    | PCA9685 Driver   |   |
+--------+----------+           |                      | (Servos, Motors) |   |
         | (RC Signal)          v                      +------------------+   |
         +-------------> [RC Model / Vehicle] <-------------------------------+
                                |
                                +-- [ER5C V2 Mod: IMU + GPS Telemetry]
                                +-- [Tier 6 Watchdog & MUX + RPi 5 SLAM]
```

---

## 🗂️ Module Descriptions (Tiers & Mods)

### 🎮 EdgeTX Mod: CRSF Trainer over USB-VCP (RadioMaster MT12 & Pocket)
*Location:* [`[EdgeTX CRSF VCP mod (MT12 & Pocket)]`](./EdgeTX%20CRSF%20VCP%20mod%20%28MT12%20%26%20Pocket%29/)

A custom EdgeTX firmware modification enabling **full-duplex CRSF over a single USB-C cable**:
*   **Channels 1–16 (100–250 Hz RX):** Control frames from PC (wheel, pedals, GCS) stream straight into the radio mixer.
*   **Full-Duplex Telemetry (TX to PC):** Live model telemetry (Battery, RSSI/LQ, GPS, and IMU) is mirrored via USB to the PC, powering FFB and dashboard instruments.
*   **Zero Dongles:** Direct USB-C connection without any external adapters, Arduino boards, or trainer cables.

---

### 📟 Tier 1: Serial to PPM Converter (Arduino & RP2350)
*Location:* [`[Arduino Tier 1]`](./Arduino%20Tier%201/) & [`[RP2350 Tier 1]`](./RP2350%20Tier%201/)

A communication bridge converting serial text commands from the GCS into a standard PPM (Pulse Position Modulation) signal.
*   **Communication:** USB Serial (`115200 bps`), data format: `ch1,ch2,...,ch8\n`.
*   **Physical Failsafe (Signal Kill):** If no valid control frames arrive for over `500 ms`, Timer interrupts are disabled and the PPM pin switches to high impedance (`INPUT`).
*   **Hardware E-STOP:** Instant `ESTOP` command overrides and kills PPM output on emergency stop (SPACEBAR).
*   **RP2350 Tier 1:** Dual-core ARM Cortex-M33 high-speed USB CDC bridge with zero-latency hardware PWM generation.

### 🌐 Tier 2: Wireless Control Hub (ESP32)
*Location:* [`[ESP32 Tier 2]`](./ESP32%20Tier%202/)

Advanced wireless control, vision, and telemetry hub based on ESP32 microcontrollers:
*   **Tier 2 Pro (V4 CRSF Multi-Link Hub):**
    - **Native CRSF Protocol:** Full support for `0x16 RC_CHANNELS_PACKED` (16 channels 11-bit) and telemetry uplink (`0x1E Attitude IMU`, `0x02 GPS`, `0x08 Battery`, `0x14 Link Stats`) with hardware-verified **CRC8 DVB-S2**.
    - **ESP-NOW Radio Link (1–2 ms Latency):** Direct connection-less MAC layer transmission with the PC USB Dongle – no Wi-Fi router needed, immune to home network congestions.
    - **FreeRTOS Dual-Core Architecture:** Full separation of radio communication & telemetry (Core 0) from the real-time PWM servo loop (Core 1, 200 Hz).
    - **Hardware Fail-Safe:** Hard neutral override (`1500 µs`) within 150 ms upon packet loss or DISARM switch (Channel 5 / AUX1).
    - **PC USB Dongle Transmitter:** Dedicated firmware turning a secondary ESP32 into a plug-and-play USB bridge for RCSIM GCS.
*   **Tier 2 Standard (V3 Full OSD & GPS / V2 / V1):**
    - **PCA9685 Hardware Autocalibration:** Dynamic closed-loop tuning of oscillator frequency with tolerance <3 µs.
    - **MJPEG FPV Streaming:** HTTP streaming on port 81 decoupled on Core 0.
    - **UDP Control & JSON Telemetry:** UDP ports 12345 (commands) and 12347 (telemetry).

---

### 📡 Tier 3 Mod: ExpressLRS RadioMaster ER Series (ER4 / ER5 / ER6 / ER8 – GPS + IMU All-in-One)
*Location:* [`[ExpressLRS ER Series Tier 3 mod]`](./ExpressLRS%20ER%20Series%20Tier%203%20mod/)

Custom ExpressLRS receiver firmware tailored for RC car dynamics and telemetry:
*   **I2C MPU6050/MPU9250 IMU:** High-speed accelerometer and gyroscope streaming (CRSF frame `0x86`).
*   **UART GPS Integration:** NMEA GPS parsing (speed, heading, latitude/longitude) with dynamic auto-baudrate.
*   **ESC Pin Protection:** Native `SERIAL_RX_ONLY` mode freeing up GPIO1 (CH2) as a standard PWM output for ESC throttles.

---

### 🛡️ Tier 6: Hardware Watchdog & SBUS Muxer (ESP32)
*Location:* `[ESP32 Tier 6 Watchdog for RPi]`

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

This project is licensed under the **GPL License**. For details, see the [LICENSE] file.
