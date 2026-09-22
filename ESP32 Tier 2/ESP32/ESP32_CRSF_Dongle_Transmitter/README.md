# RCSIM - PC USB to ESP-NOW CRSF Dongle (Transmitter)

## 📌 Overview

This firmware turns a secondary **ESP32 microcontroller** into a dedicated **USB-to-ESP-NOW Radio Dongle** for the **RCSIM Ground Control Station (GCS)** running on a PC.

### Full-Duplex Architecture:
1. **Control Stream (PC -> RC Model):**
   - The PC GCS generates native **CRSF** packets (`0x16 RC_CHANNELS_PACKED` with 16x 11-bit channels at 115200 bps) over a virtual USB COM port.
   - The ESP32 Dongle reads these frames and broadcasts them instantly over **ESP-NOW** with **1–2 ms latency**.
2. **Telemetry Uplink (RC Model -> PC):**
   - The vehicle streams CRSF telemetry frames (`0x1E Attitude`, `0x02 GPS`, `0x08 Battery`, `0x14 Link Stats`) back to the Dongle.
   - The Dongle pushes telemetry frames directly over USB to the PC, powering real-time **Force Feedback (FFB)** on sim-racing steering wheels and instrument clusters.

---

## 🛠️ Flashing & Running

1. Connect the ESP32 (e.g., ESP32-WROOM-32, NodeMCU, ESP32-S3) via USB to your PC.
2. Open `ESP32_CRSF_Dongle_Transmitter.ino` in Arduino IDE.
3. Select board **`ESP32 Dev Module`** and the correct COM port.
4. Click **Upload** (`Ctrl + U`).
5. In **RCSIM GCS**:
   - Connection Settings: Select **CRSF Direct** (or Tier 3).
   - Port: Select the COM port of your Dongle (e.g., `COM7`).
   - Baudrate: `115200`.
   - Click **Connect** and **ARM**!
