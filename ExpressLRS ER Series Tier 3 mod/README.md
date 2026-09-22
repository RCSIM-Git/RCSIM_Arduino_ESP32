# 🛰️ RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Custom Firmware & Telemetry Mods (GPS + IMU)

*Languages & Manuals:*
- **GPS + IMU All-In-One (Recommended):** [Polski 🇵🇱](README_PL_RadioMaster_ER5Cv2_GPS.md) | [English 🇬🇧](README_EN_RadioMaster_ER5Cv2_GPS.md)
- **IMU Dedicated (MPU9250):** [Polski 🇵🇱](README_PL_RadioMaster_ER5Cv2_IMU.md) | [English 🇬🇧](README_EN_RadioMaster_ER5Cv2_IMU.md)
- **Source Code Developer Patch Guides:** [GPS Patch Guide](PATCH_INSTRUCTIONS_GPS.md) | [IMU Patch Guide](PATCH_INSTRUCTIONS.md)
- **Unified Git Patch (GPLv3 Source):** [`elrs_v4.1_er5cv2_gps_imu.patch`](./elrs_v4.1_er5cv2_gps_imu.patch)

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

---

## 🩺 Real-Time Hardware IMU Diagnostics (I2C Error Codes)

When the receiver cannot physically establish communication with the IMU chip (`!imu_initialized`), it sends real-time diagnostic status codes inside frame `0x86`:

| Axis in GCS (`Raw`) | Firmware Variable | Value | Meaning & Diagnosis |
| :--- | :--- | :---: | :--- |
| **Acceleration X** | `diag_err68` | **`0`**<br>**`2`**<br>**`3`**<br>**`4`**<br>**`99`**<br>**`255`** | **0** = I2C ACK OK<br>**2** = **Device not responding at 0x68 (NACK on address)**<br>**3** = NACK during data transfer<br>**4** = Other bus error (e.g. short circuit)<br>**99** = Idle state (read not attempted)<br>**255** = I2C bus disabled (no SDA/SCL pins set in WebUI) |
| **Acceleration Y** | `diag_whoami68` | **`0x71`** (113)<br>**`0x73`** (115)<br>**`0x68`** (104)<br>**`0x70`** (112)<br>**`0`** | **MPU-9250**<br>**MPU-9255**<br>**MPU-6050**<br>**MPU-6500**<br>**0 = WHO_AM_I register read failed** |
| **Acceleration Z** | Default vector | **`16384`** | Gravity constant $1.0g$ ($9.81 m/s^2$) – safe heartbeat signal |
| **Angular velocity X** | `diag_err69` | Same as `diag_err68` | Probing alternative I2C address `0x69` (when AD0 = VCC) |
| **Angular velocity Y** | `diag_whoami69` | Chip IDs | Read WHO_AM_I register at address `0x69` |
| **Angular velocity Z** | `imu_address` | **`104`** (`0x68`)<br>**`105`** (`0x69`) | Currently probed I2C address |

---

## 🛰️ Real-Time Hardware GPS Diagnostics (UART / NMEA Status Codes)

When the receiver has not yet locked valid NMEA sentences (`validPacketsCount == 0`) or if signal stream is lost (> 3s), the ER5C V2 periodically emits a diagnostic frame `0x02` (`CRSF_FRAMETYPE_GPS`) with `Latitude = 0` and `Longitude = 0` at 1Hz:

| CRSF Field (`0x02`) | EdgeTX Sensor | RCSIM Telemetry Field | Diagnostic Value & Meaning |
| :--- | :--- | :--- | :--- |
| **`Satellites`** | `Sats` | `gps["satellites"]`<br>`diagnostic["state_code"]` | **GPS Connection State:**<br>• **`0` (`NO_DATA`):** 0 bytes received on RX (CH3). Line completely silent.<br>• **`1` (`SCANNING_BAUD`):** Bytes arriving on RX, auto-baud scanning UART rates.<br>• **`2` (`CHECKSUM_FAIL`):** Bytes arriving, but sentences fail NMEA checksum.<br>• **`3` (`BAUD_LOCKED`):** Baud locked, awaiting full `$xxGGA` / `$xxRMC` sentences.<br>• **`4` (`LOST_TIMEOUT`):** Stream lost during operation (no data for > 3s). |
| **`Groundspeed`** | `GSpd` | `gps["speed"]`<br>`diagnostic["baud_rate"]` | **Currently Probed / Locked Baudrate:**<br>• **`115.2 km/h`** = **115200 bps**<br>• **`9.6 km/h`** = **9600 bps**<br>• **`38.4 km/h`** = **38400 bps**<br>• **`57.6 km/h`** = **57600 bps** |
| **`Altitude`** | `GAlt` | `gps["altitude"]`<br>`diagnostic["bytes_received"]` | **Raw Byte Counter on CH3** (`rawBytesCount % 10000`):<br>• **`0 m`** = Zero bytes received (hardware disconnect / dead module).<br>• **`> 0 m`** (e.g. 45 m, 120 m...) = Number of bytes physically received. If incrementing, wiring and GPS power are healthy! |
| **`Heading`** | `Hdg` | `gps["heading"]`<br>`diagnostic["csum_errors"]` | **NMEA Checksum Error Counter** (`csumErrors / 100.0`):<br>• Shows count of malformed / corrupt lines received. |

---

## 📜 Source Code, Git Patch & GNU GPLv3 License

- **Base Project:** [ExpressLRS/ExpressLRS](https://github.com/ExpressLRS/ExpressLRS) (branch `master`, base commit `5909f77`).
- **GPLv3 Compliance (Corresponding Source):** In full compliance with Section 6 of the GNU General Public License v3.0, the complete set of C++ and WebUI modifications (`src/lib/IMU/`, `SerialGPS` auto-baudrate, ESC PWM safety on CH2/GPIO1, and WebUI uncoupling) is distributed as a unified git patch:
  [`elrs_v4.1_er5cv2_gps_imu.patch`](./elrs_v4.1_er5cv2_gps_imu.patch)
- **Applying Patch to Clean ExpressLRS Source:**
  ```bash
  git clone https://github.com/ExpressLRS/ExpressLRS.git
  cd ExpressLRS
  git checkout 5909f77
  git apply elrs_v4.1_er5cv2_gps_imu.patch
  ```
- **License:** GNU General Public License v3.0 (GPLv3).
- **Disclaimer:** This software is an independent community modification developed for the RCSIM project and is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. ExpressLRS and RadioMaster are registered or unregistered trademarks of their respective owners.

