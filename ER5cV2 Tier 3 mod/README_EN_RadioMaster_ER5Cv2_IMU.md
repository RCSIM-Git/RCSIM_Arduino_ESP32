# 🚀 RadioMaster ER5C V2 ExpressLRS (2.4GHz) – IMU Telemetry Mod (MPU9250 / MPU6050 / MPU6500)

*Languages: [Polski](README_PL_RadioMaster_ER5Cv2_IMU.md) | [English](README_EN_RadioMaster_ER5Cv2_IMU.md)*

---

Extended firmware build of **ExpressLRS v4.1.0** tailored for the **RadioMaster ER5C V2 (ESP8285)** receiver.
This custom firmware adds native I2C support for **MPU9250 / MPU6050 / MPU6500 / MPU9255** inertial measurement units (IMU), transmitting telemetry data **in real time (20Hz)** to the transmitter module (e.g. RadioMaster Nomad) and the host PC (RCSIM Ground Control Station).

---


## 📌 Key Features

- **Raw Inertial & Magnetic Telemetry:** Accelerometer ($\pm 2g$), Gyroscope ($\pm 250\,\text{dps}$), Magnetometer ($\mu\text{T}$).
- **Custom CRSF Frame:** Telemetry frame `0x86` (`CRSF_FRAMETYPE_CUSTOM_IMU`), extending the standard CRSF v3 protocol.
- **20Hz Radio Stream:** Low-latency telemetry stream transmitted directly from RX to TX and forwarded over serial/USB to the PC.
- **Failsafe Heartbeat:** If the IMU sensor is disconnected, the receiver broadcasts a verification heartbeat frame with a default gravity vector of $1.0g$ ($9.81\,\text{m/s}^2$).
- **Dynamic I2C Pin Assignment:** Configure arbitrary I2C pins (SDA / SCL) directly in the receiver's WebUI.

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

> 💡 **Note:** Ensure the `AD0` address pin on your MPU9250 breakout board is pulled to GND (default I2C address `0x68`).

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
3. Select the file: `ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin.gz`.
4. Click **Update** and wait for the flash process to finish and the device to restart.

### Method B: Wired via FTDI USB-UART Adapter
1. Press and hold the **BOOT** pad/button on the underside of the ER5C V2 receiver while connecting the FTDI adapter to your PC.
2. Run `esptool.py` (replace `COMX` with your serial COM port):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin
```

---

## 💻 Python / RCSIM Integration (CRSF 0x86 Decoder)

In your ground station telemetry decoder (e.g. `crsf_transceiver.py`), handle frame type `0x86` as follows:

```python
import struct

# Inside your CRSF telemetry packet parsing loop:
elif frame_type == 0x86 and len(payload) >= 18:  # CRSF_FRAMETYPE_CUSTOM_IMU
    ax, ay, az, gx, gy, gz, mx, my, mz = struct.unpack(">hhhhhhhhh", payload[0:18])
    
    # Convert raw LSBs to engineering units:
    ax_g, ay_g, az_g = ax / 16384.0, ay / 16384.0, az / 16384.0  # g (+-2g range)
    gx_d, gy_d, gz_d = gx / 131.0, gy / 131.0, gz / 131.0        # deg/s (+-250dps range)
    mx_u, my_u, mz_u = mx * 0.15, my * 0.15, mz * 0.15           # uT
    
    data["imu"] = {
        "ax": ax_g, "ay": ay_g, "az": az_g,
        "gx": gx_d, "gy": gy_d, "gz": gz_d,
        "mx": mx_u, "my": my_u, "mz": mz_u,
        "accel_x": ax_g, "accel_y": ay_g, "accel_z": az_g,
        "gyro_x": gx_d, "gyro_y": gy_d, "gyro_z": gz_d,
        "mag_x": mx_u, "mag_y": my_u, "mag_z": mz_u,
        "raw": {
            "ax": ax, "ay": ay, "az": az,
            "gx": gx, "gy": gy, "gz": gz,
            "mx": mx, "my": my, "mz": mz,
        }
    }
```

---

## 🛠️ Release Files

- `ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin.gz` – Compressed binary ready for WebUI OTA flashing.
- `ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin` – Raw binary file for FTDI UART flashing with `esptool.py`.
- `README_PL_RadioMaster_ER5Cv2_IMU.md` – Documentation (Polish version).
- `README_EN_RadioMaster_ER5Cv2_IMU.md` – Documentation (English version).
