# 🚀 RadioMaster ER5C V2 ExpressLRS (2.4GHz) – GPS + IMU Telemetry Mod (MPU9250 / MPU6050 / MPU6500)

*Languages: [Polski](README_PL_RadioMaster_ER5Cv2_IMU.md) | [English](README_EN_RadioMaster_ER5Cv2_IMU.md)*

---

Extended firmware build of **ExpressLRS v4.1.0** tailored for the **RadioMaster ER5C V2 (ESP8285)** receiver.
This custom firmware adds native I2C support for **MPU9250 / MPU6050 / MPU6500 / MPU9255** inertial measurement units (IMU), transmitting telemetry data **in real time; the IMU loop runs every 10 ms (100 Hz), while received sample rate depends on telemetry bandwidth** to the transmitter module (e.g. RadioMaster Nomad) and the host PC (RCSIM Ground Control Station).

---


## 📌 Key Features

- **Combined GPS + IMU firmware:** UART GPS (NMEA / UBX) and I2C IMU operate together.
- **Measured acceleration and angular rates:** MPU hardware ranges are ±2 g and ±250 deg/s; standard CRSF `0x13` encodes ±16 g and ±2000 deg/s.
- **Standard CRSF frames:** `0x13` carries timestamp, gyro, acceleration and temperature; `0x1E` carries separate estimated orientation. GPS uses `0x02` and GPS time `0x03`.
- **No fabricated raw samples:** Failed I2C reads do not produce `0x13` measurements. The frame does not contain magnetometer data.
- **Dynamic I2C assignment:** Configure SDA/SCL in receiver WebUI. Actual radio sample rate must be checked in RCSIM, not inferred from link packet rate.

---

## 🔌 Hardware Wiring (Pinout)

The **RadioMaster ER5C V2** receiver provides 5 PWM servo output headers. We assign **Channel 4 (CH4)** and **Channel 5 (CH5)** signal pins as the I2C bus interface for the IMU sensor.

### Pinout Table:

| IMU Pin (MPU9250) | RadioMaster ER5C V2 Header | WebUI Pin Function | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **`+`** (PWM Power Rail) | Power (3.3V / 5V) | Power supply |
| **GND** | **`-`** (PWM Ground Rail) | Ground (GND) | Ground |
| **SDA** | **`~` (Signal Pin CH4)** | **`I2C SDA`** | I2C Data Line |
| **SCL** | **`~` (Signal Pin CH5)** | **`I2C SCL`** | I2C Clock Line |

> 💡 **Note:** Both I2C addresses **`0x68`** (AD0=GND) and **`0x69`** (AD0=HIGH, e.g. on SEN-MPU6050) are auto-detected and supported out of the box!

---

## 🛰️ GPS wiring alongside IMU

- GPS TX → **CH3 signal (GPIO3)**; set Output 3 to **Serial RX**, and the primary **Serial Protocol** to **GPS**.
- Keep **CH2 (GPIO1)** in PWM mode to preserve the ESC output. In receive-only mode GPS RX remains disconnected; automatic configuration commands cannot reach GPS, so it must already emit supported NMEA or UBX data.
- CH1 remains steering PWM; CH4 (GPIO9) is IMU SDA and CH5 (GPIO10) IMU SCL.
- All modules need common GND. Choose module supply voltage according to its specification; PWM rail voltage is not automatically safe for a bare MPU. I2C signal pull-ups must use 3.3 V.
- Save and restart. Verify GPS frames `0x02` and IMU frames `0x13` independently; a GPS fix requires satellite reception and is not needed for IMU samples.

---

## ⚙️ WebUI Configuration Guide

1. Power on the ER5C V2 receiver and wait ~60 seconds without turning on the transmitter to enter Wi-Fi AP mode.
2. Connect your PC or smartphone to the receiver's Wi-Fi hotspot:
   - **SSID:** `ExpressLRS RX`
   - **Password:** `expresslrs`
3. Open a web browser and navigate to: **`http://10.0.0.1`** (or `http://elrs_rx.local`).
4. Navigate to the **Connections / PWM Pin Functions** section:
   - Set **Output 4** function to: **`I2C SDA`**
   - Set **Output 5** function to: **`I2C SCL`**
5. Click **SAVE** at the bottom of the page and reboot the receiver.

---

## 📥 Firmware Flashing Guide

### Method A: Over-The-Air (OTA) via WebUI (Recommended)
1. Connect to the receiver's WebUI (`http://10.0.0.1` or `http://elrs_rx.local`).
2. Navigate to the **Update** tab (`http://10.0.0.1/#update`).
3. Select the file: `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz`.
4. Click **Update** and wait for the flash process to finish and the device to restart.

### Method B: Wired via FTDI USB-UART Adapter
1. Press and hold the **BOOT** pad/button on the underside of the ER5C V2 receiver while connecting the FTDI adapter to your PC.
2. Run `esptool.py` (replace `COMX` with your serial COM port):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin
```

---

## 💻 RCSIM / CRSF 0x13

Current RCSIM already decodes `0x13`; do not add the obsolete `0x86` decoder. Verify `Raw Crsf → 0x13`, an 18-byte payload, `source: crsf_raw`, temperature and changing gyro values. `0x1E` alone is orientation, not measured raw IMU. The complete RX → TX → USB/serial path must forward `0x13`.

Reference payload decoding (big-endian; gyro X converted from CRSF left-positive to RCSIM right-positive):

```python
import struct

# Only after CRSF frame length and CRC validation:
if frame_type == 0x13 and len(payload) == 18:
    timestamp, gx, gy, gz, ax, ay, az, temp = struct.unpack(">I7h", payload)
    accel_g = tuple(v * 16.0 / 32767.0 for v in (ax, ay, az))
    gyro_dps = (-gx * 2000.0 / 32767.0,
                gy * 2000.0 / 32767.0, gz * 2000.0 / 32767.0)
    temperature_c = temp / 100.0
```

---

## 🛠️ Release Files

- `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz` – Compressed binary ready for WebUI OTA flashing.
- `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin` – Raw binary file for FTDI UART flashing with `esptool.py`.
- `README_PL_RadioMaster_ER5Cv2_IMU.md` – Documentation (Polish version).
- `README_EN_RadioMaster_ER5Cv2_IMU.md` – Documentation (English version).
