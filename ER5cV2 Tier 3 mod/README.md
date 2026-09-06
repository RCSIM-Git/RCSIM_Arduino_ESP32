# 🛰️ RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Custom Firmware & Telemetry Mods (GPS + IMU)

*Languages & Manuals:*
- **GPS + IMU All-In-One (Recommended):** [Polski 🇵🇱](README_PL_RadioMaster_ER5Cv2_GPS.md) | [English 🇬🇧](README_EN_RadioMaster_ER5Cv2_GPS.md)
- **IMU Dedicated (MPU9250):** [Polski 🇵🇱](README_PL_RadioMaster_ER5Cv2_IMU.md) | [English 🇬🇧](README_EN_RadioMaster_ER5Cv2_IMU.md)
- **Source Code Developer Patch Guides:** [GPS Patch Guide](PATCH_INSTRUCTIONS_GPS.md) | [IMU Patch Guide](PATCH_INSTRUCTIONS.md)

---

## 📖 Overview

Custom builds of **ExpressLRS v4.1.0** tailored specifically for the 5-channel PWM receiver **RadioMaster ER5C V2 (ESP8285)**.
These builds unlock real-time vehicle telemetry for autonomous and teleoperated RC vehicles (RCSIM Tier 3 architecture):
- **GPS Telemetry:** Position, speed, altitude, heading, satellite count, and precise UTC time directly over standard CRSF frames (`0x02` and `0x03`).
- **Inertial Telemetry (IMU):** 9-DoF acceleration, angular velocity, and magnetic field from MPU9250 / MPU6050 sensors over custom CRSF frame `0x86`.
- **PWM Motor & Steering Integrity:** Intelligent `SERIAL_RX_ONLY` driver mode ensures CH2 remains a fully functional PWM output for the motor ESC, while CH3 serves as the GPS UART input.

---

## 🛠️ Available Firmware Releases

| Firmware File | Type | Flashing Method | Features | Recommended Use |
| :--- | :--- | :--- | :--- | :--- |
| **`ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz`** | All-In-One | **OTA Wi-Fi WebUI** | GPS + IMU + PWM | **Standard Tier 3 (Recommended)** |
| **`ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin`** | All-In-One | **FTDI UART (`esptool.py`)** | GPS + IMU + PWM | Wired recovery / initial flash |
| **`ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin.gz`** | IMU Dedicated | **OTA Wi-Fi WebUI** | IMU + PWM | Setup with IMU only |
| **`ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin`** | IMU Dedicated | **FTDI UART (`esptool.py`)** | IMU + PWM | Wired recovery |

---

## 🔌 Hardware Wiring Comparison

### 1. All-In-One Setup (GPS + IMU + Motor + Steering):
| ER5C V2 Header | Connected Hardware | Module Pin | WebUI Configuration | Function |
| :--- | :--- | :--- | :--- | :--- |
| **CH1** | Steering Servo | Signal | **50Hz - 333Hz PWM** | Steering Angle Control |
| **CH2** | Motor ESC | Signal | **50Hz - 400Hz PWM** | Throttle / Motor Control |
| **CH3** | GPS Module (Beitian BN-220, etc.) | **TXD** of GPS | **Serial RX** | NMEA In (Auto-baud 9600-115200) |
| **CH4** | IMU Breakout (MPU9250) | **SDA** | **I2C SDA** | I2C Data Line |
| **CH5** | IMU Breakout (MPU9250) | **SCL** | **I2C SCL** | I2C Clock Line |
| **`+` / `-`** | Power Rail | VCC / GND | 5V BEC from ESC | Common Power & Ground |

### 2. IMU-Only Setup (Steering + Throttle + AUX):
| ER5C V2 Header | Connected Hardware | Module Pin | WebUI Configuration | Function |
| :--- | :--- | :--- | :--- | :--- |
| **CH1** | Steering Servo | Signal | **50Hz - 333Hz PWM** | Steering Angle Control |
| **CH2** | Motor ESC | Signal | **50Hz - 400Hz PWM** | Throttle / Motor Control |
| **CH3** | AUX Servo / Light | Signal | **50Hz PWM** | Auxiliary channel |
| **CH4** | IMU Breakout (MPU9250) | **SDA** | **I2C SDA** | I2C Data Line |
| **CH5** | IMU Breakout (MPU9250) | **SCL** | **I2C SCL** | I2C Clock Line |

---

## ⚙️ Quick WebUI Setup

1. Power on the ER5C V2 receiver and wait ~60s without transmitter connection until the LED fast-blinks (Wi-Fi AP mode).
2. Connect to `ExpressLRS RX` Wi-Fi network (password: `expresslrs`).
3. Open browser at `http://10.0.0.1` (or `http://elrs_rx.local`).
4. Set pin functions in **Connections / PWM Pin Functions**:
   - **Output 1**: `50Hz PWM`
   - **Output 2**: `50Hz PWM`
   - **Output 3**: `Serial RX` *(for GPS)*
   - **Output 4**: `I2C SDA` *(for IMU)*
   - **Output 5**: `I2C SCL` *(for IMU)*
5. In **Serial/UART Options**, set **Serial 1 Protocol** to `GPS`.
6. Click **SAVE** and power-cycle receiver.

---

## 📥 Flashing Instructions

### Method A: Over-The-Air (OTA) via Wi-Fi WebUI (Recommended)
1. Navigate to `http://10.0.0.1/#update`.
2. Under **Firmware Update**, select `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz`.
3. Click **Update** and wait ~20 seconds for verification and automatic reboot.

### Method B: FTDI USB-UART Flasher (esptool.py)
1. Hold down the **BOOT** pad on the underside of the receiver while plugging FTDI into USB.
2. Run:
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin
```

---

## 💻 Python Telemetry Decoder Integration

```python
import struct

def parse_crsf_telemetry(frame_type: int, payload: bytes, data: dict):
    # CRSF_FRAMETYPE_GPS (0x02)
    if frame_type == 0x02 and len(payload) >= 15:
        lat, lon, speed, heading, alt = struct.unpack(">iiHHH", payload[0:14])
        data["gps"] = {
            "lat": lat / 1e7,
            "lon": lon / 1e7,
            "speed_kmh": speed / 10.0,
            "heading_deg": heading / 100.0,
            "altitude_m": alt - 1000,
            "satellites": payload[14]
        }

    # CRSF_FRAMETYPE_GPS_TIME (0x03)
    elif frame_type == 0x03 and len(payload) >= 9:
        year, month, day, hour, minute, second, ms = struct.unpack(">hBBBBBH", payload[0:9])
        data["gps_time"] = {
            "year": year, "month": month, "day": day,
            "hour": hour, "minute": minute, "second": second,
            "millisecond": ms
        }

    # CRSF_FRAMETYPE_CUSTOM_IMU (0x86)
    elif frame_type == 0x86 and len(payload) >= 18:
        ax, ay, az, gx, gy, gz, mx, my, mz = struct.unpack(">hhhhhhhhh", payload[0:18])
        data["imu"] = {
            "accel_g": (ax / 16384.0, ay / 16384.0, az / 16384.0),
            "gyro_dps": (gx / 131.0, gy / 131.0, gz / 131.0),
            "mag_ut": (mx * 0.15, my * 0.15, mz * 0.15)
        }
```
