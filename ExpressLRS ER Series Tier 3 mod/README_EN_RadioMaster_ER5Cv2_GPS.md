# 🛰️ RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Custom Firmware with GPS + IMU Telemetry (All-In-One)

*Available languages: [Polski](README_PL_RadioMaster_ER5Cv2_GPS.md) | [English](README_EN_RadioMaster_ER5Cv2_GPS.md)*

---

Extended **ExpressLRS v4.1.0** firmware designed for the **RadioMaster ER5C V2 (ESP8285)** 5-channel PWM receiver.
This firmware brings native support for **GPS modules (UART NMEA / UBX)** and **IMU inertial sensors (MPU9250 / MPU6050)**, transmitting real-time vehicle telemetry directly to the **RCSIM GCS** ground station and **RCSIM MCS** mobile control station.

---

## 📌 Key Features

1. **GPS UART support:** NMEA (GGA/RMC/VTG) and u-blox UBX navigation data. Auto-baud candidates: **115200, 9600, 38400, 57600, 230400, 19200 bps**. Compatibility depends on the module's actual output protocol; brand alone is not a guarantee.
2. **Enhanced NMEA Parser (RMC + GGA + VTG):**
   - Parses latitude, longitude, altitude, and satellite count from `$xxGGA`.
   - Directly parses groundspeed and track course heading from `$xxRMC` (critical for budget modules that do not emit `$xxVTG` by default).
3. **True All-In-One Architecture (GPS + IMU + PWM):**
   - **CH1 (GPIO0):** Dedicated PWM output for steering servo.
   - **CH2 (GPIO1):** Independent PWM output for motor ESC. An intelligent `SERIAL_RX_ONLY` driver patch prevents the UART from seizing the TX pin, keeping it 100% available for PWM generation.
   - **CH3 (GPIO3):** UART0 RX input pin for the GPS module TX wire.
   - **CH4 (GPIO9):** I2C SDA for MPU9250 / MPU6050 IMU (or auxiliary PWM output).
   - **CH5 (GPIO10):** I2C SCL for MPU9250 / MPU6050 IMU (or auxiliary PWM output).
4. **Standard and Extended CRSF Frames:**
   - **`0x02` (`CRSF_FRAMETYPE_GPS`):** Latitude/Longitude ($10^{-7\circ}$), groundspeed ($0.1$ km/h), heading ($0.01^\circ$), altitude (offset 1000m), satellite count.
   - **`0x03` (`CRSF_FRAMETYPE_GPS_TIME`):** Precise UTC time, date, and milliseconds.
   - **`0x13`:** Timestamp, angular rates, acceleration and temperature; no magnetic field. Separate `0x1E` carries estimated orientation.

---

## 🔌 Hardware Wiring Diagram (Pinout)

The **RadioMaster ER5C V2** receiver features 5 standard 3-pin RC servo pin headers (Signal, +, -).

### Recommended Wiring Table for RC Vehicles (Tier 3):

| ER5C V2 Header | Connected Hardware | Module Pin | WebUI Pin Mode | Function |
| :--- | :--- | :--- | :--- | :--- |
| **CH1** | Steering Servo | Signal (white/yellow) | **PWM (50Hz - 333Hz)** | Steering Angle Control |
| **CH2** | Motor ESC | Signal (white) | **PWM (50Hz - 400Hz)** | Throttle / Brake Control |
| **CH3** | GPS Module (e.g. BN-220) | **TXD** of GPS module | **Serial RX** | Receiving NMEA sentences |
| **CH4** | IMU (MPU9250) | **SDA** | **I2C SDA** | Gyro/Accel data line |
| **CH5** | IMU (MPU9250) | **SCL** | **I2C SCL** | I2C Clock line |
| **`+` & `-` Pins** | Power Rails | VCC / GND | 5V BEC from ESC | Shared Power & Ground |

> 💡 **GPS Wiring Tips:**
> - Only connect the **TX wire** (yellow/green) from the GPS module to the **CH3 signal pin**. The GPS RX wire can remain disconnected.
> - Connect the GPS module power (VCC and GND) to any unused `+` and `-` pins on the servo rail.

---

## ⚙️ WebUI Configuration Guide

1. Power on the ER5C V2 receiver and wait ~60 seconds without turning on your transmitter until the LED fast-blinks (Wi-Fi Access Point mode).
2. Connect to the receiver's Wi-Fi network:
   - **SSID:** `ExpressLRS RX`
   - **Password:** `expresslrs`
3. Open your browser and navigate to: **`http://10.0.0.1`** (or `http://elrs_rx.local`).
4. Navigate to **Connections / PWM Pin Functions**:
   - For **Output 1**: Select **`50Hz PWM`** (or desired steering frequency).
   - For **Output 2**: Select **`50Hz PWM`** (or desired ESC frequency).
   - For **Output 3**: Select **`Serial RX`**.
   - *(Optional)* For **Output 4**: Select **`I2C SDA`** (if using MPU9250).
   - *(Optional)* For **Output 5**: Select **`I2C SCL`** (if using MPU9250).
5. In the **Serial/UART Options** panel:
   - Set **Serial Protocol** to: **`GPS`**.
6. Click **SAVE** at the bottom of the tables and reboot the receiver.

---

## 📥 Firmware Flashing Instructions

### Method A: Wireless OTA via Wi-Fi WebUI (Recommended)
1. Connect to the WebUI (`http://10.0.0.1/#update`).
2. Under **Firmware Update**, select the appropriate package:
   - **`ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz`** (Recommended for RCSIM – simultaneous GPS and IMU AHRS support)
   - **`ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin.gz`** (Dedicated GPS only – clean PR #3798 without IMU code)
3. Click **Update** and wait ~20 seconds for the flash to finish and the device to reboot.

### Method B: FTDI USB-UART Flasher (esptool.py)
1. Hold down the **BOOT** button on the bottom of the ER5C V2 while plugging the FTDI adapter into your computer's USB port.
2. Run in terminal (replace `COMX` with your FTDI port):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin
```
*(or `ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin` for GPS-only variant)*

---

## 💻 Python / RCSIM Integration (CRSF 0x02 & 0x03 Decoder)

In the RCSIM system, GPS telemetry is natively parsed in `pc_app/core/comm/crsf_transceiver.py`:

```python
import struct

# In CRSF telemetry receive loop:
if frame_type == 0x02 and len(payload) >= 15:  # CRSF_FRAMETYPE_GPS
    lat, lon, speed, heading, alt = struct.unpack(">iiHHH", payload[0:14])
    sats = payload[14]
    data["gps"] = {
        "lat": lat / 1e7,
        "lon": lon / 1e7,
        "speed": speed / 10.0,       # km/h
        "heading": heading / 100.0,  # deg
        "altitude": alt - 1000,      # m
        "satellites": sats,
    }

elif frame_type == 0x03 and len(payload) >= 9:  # CRSF_FRAMETYPE_GPS_TIME
    year, month, day, hour, minute, second, ms = struct.unpack(">hBBBBBH", payload[0:9])
    data["gps_time"] = {
        "year": year, "month": month, "day": day,
        "hour": hour, "minute": minute, "second": second,
        "millisecond": ms
    }
```

---

## 🩺 Verification and troubleshooting

The combined firmware includes GPS UART and CRSF `0x13` IMU in the same build. The older `MPU9250_CRSF13` package name did not imply GPS code was absent; pin mapping and Serial Protocol must still be configured.

- GPS: verify receiver WebUI GPS status (protocol, baud, sample age, fix and satellites), then CRSF `0x02` in RCSIM. Baud detection is not a satellite fix.
- IMU: verify `0x13` payload 18 bytes and `source: crsf_raw`. Failed I2C reads do not create artificial raw measurements or a 1 g heartbeat. See [IMU setup and decoding](README_EN_RadioMaster_ER5Cv2_IMU.md).
- Receive-only GPS: CH3 must be Serial RX, primary Serial Protocol must be GPS, CH2 stays PWM. GPS TX → CH3; GPS RX disconnected. The GPS must already emit supported NMEA or UBX; configuration commands cannot reach it in this mode.
- Optional bidirectional GPS: map CH2/GPIO1 to Serial TX and connect it to GPS RX. This enables u-blox configuration attempts, but **CH2 can no longer drive the ESC**. Use another suitable control output if needed.
- Power: use module-rated supply voltage, common GND and 3.3 V-compatible signal levels; check labels rather than wire colours.
- If no UART data arrives, check TX→RX, CH3 mapping, Serial Protocol and power. If data arrives but detection fails, check the module output and candidate baud rates. A GPS fix requires open sky.
- Verify GPS and IMU independently. Link packet Hz does not equal fresh IMU sample Hz; IMU loop is 100 Hz, actual received rate depends on telemetry bandwidth.

---

## 🛠️ Release Artifacts

Files available in this directory:
1. `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz` – Compressed binary for WebUI OTA update.
2. `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin` – Raw binary for FTDI / `esptool.py`.
3. `README_PL_RadioMaster_ER5Cv2_GPS.md` – Technical manual (Polish).
4. `README_EN_RadioMaster_ER5Cv2_GPS.md` – Technical manual (English).
