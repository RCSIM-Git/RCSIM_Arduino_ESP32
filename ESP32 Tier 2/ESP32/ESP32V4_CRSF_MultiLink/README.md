# RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub)

## 📌 Overview

The **Tier 2 Pro (V4)** firmware for the **ESP32** microcontroller serves as an advanced onboard vehicle controller and telemetry hub for RC vehicles and rovers, featuring:
- **Native CRSF Protocol (Crossfire / ExpressLRS)** with hardware-verified **CRC8 DVB-S2** (polynomial `0xD5`).
- **Multi-layer Radio Communication**:
  - **ESP-NOW Link:** Ultra-low latency of **1–2 ms**, connection-less MAC layer protocol (no Wi-Fi router required, paired via MAC or broadcast).
  - **Hardware Serial:** Direct USB CDC connection with PC or UART link to external LoRa modules (e.g., SX1262 / SX1280).
  - **UDP:** Traditional Wi-Fi network routing or local LTE/GSM bridge.
  - **MicroLink VPN (Tailscale / WireGuard):** Integrated support for [CamM2325/microlink](https://github.com/CamM2325/microlink) – secure remote driving over the Internet / 4G LTE with zero port-forwarding and no public IP needed!
- **PCA9685 I2C Servo Controller:** 16-channel PWM servo & ESC outputs running on **Fast Mode (400 kHz)** with automatic bus recovery against EMI noise from electric motors.
- **Full Sensor Telemetry Uplink (CRSF)**:
  - `0x1E Attitude`: Pitch, Roll, and Yaw angles from IMU (MPU6050 / MPU9250 with hardware DLPF filter).
  - `0x02 GPS`: Latitude, Longitude, Groundspeed in km/h, Heading, Altitude, and Satellite count.
  - `0x08 Battery`: Main pack voltage measured with exponential moving average (EMA) filter on ADC1.
  - `0x14 Link Statistics`: RSSI (dBm) and Link Quality (LQI 0–100%).
- **Hardware Fail-Safe & Dual-Core FreeRTOS**:
  - Full decoupling of the radio communication thread (**Core 0**) from the real-time PWM servo loop (**Core 1**, 200 Hz).
  - Automatic neutral override (`1500 µs`) upon 150 ms without packets or when the ARM switch (Channel 5 / AUX1) is DISARMED.

---

## 🔌 Hardware Pinout

| Peripheral / Module | ESP32 Pin | Description / Notes |
|---|---|---|
| **I2C SDA** | `GPIO 13` | I2C Data bus (PCA9685, MPU6050/9250 IMU) |
| **I2C SCL** | `GPIO 14` | I2C Clock bus (400 kHz Fast Mode) |
| **PCA Calibration** | `GPIO 12` | Optional feedback loop from PCA9685 CH15 |
| **GPS TX -> ESP32 RX** | `GPIO 32` (RX1) | NMEA Serial input (HardwareSerial 1, 9600-115200 bps) |
| **Battery (VBAT)** | `GPIO 33` (ADC1) | Voltage divider R1=10k, R2=2.2k (~5.545 ratio) |
| **Power Input** | `5V / VIN` | External 5V/2A BEC (never power solely from 3.3V) |

---

## ⚙️ Transport Selection

In `ESP32V4_CRSF_MultiLink.ino`, select your desired transport:

```cpp
// Options: TRANSPORT_ESP_NOW, TRANSPORT_SERIAL, TRANSPORT_UDP, TRANSPORT_MICROLINK_VPN
#define ACTIVE_TRANSPORT     TRANSPORT_ESP_NOW

// If using TRANSPORT_MICROLINK_VPN:
#define TAILSCALE_AUTH_KEY   "tskey-auth-YOUR_AUTH_KEY_HERE"
#define GCS_TAILSCALE_IP     IPAddress(100, 64, 0, 1) // Tailscale GCS IP
```

---

## 🛠️ Required Libraries (Arduino IDE Library Manager)

- **Adafruit PWM Servo Driver Library**
- **Adafruit BusIO**
- **TinyGPSPlus**
- **MPU6050_light**

---

## 🚀 Compiler Settings

- **Board:** `ESP32 Dev Module` or `ESP32 Wrover Module`
- **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)`
- **Upload Speed:** `115200`
