# 🎮 EdgeTX — CRSF Trainer over USB-VCP + Full-Duplex Telemetry Mirror

> [!CAUTION]
> ### ⚠️ EXPERIMENTAL & COMMUNITY BUILDS — USE AT YOUR OWN RISK
> **Important Disclaimer:**
> - These custom firmware builds are based on the development branch of EdgeTX PR **#7630** (`feat/crsf-trainer-over-usb-vcp`, commit `e5784ee5`) enhanced with a custom full-duplex telemetry mirror patch (`telemetrySetMirrorCb`).
> - **The newly added builds (Boxer, TX16S, TX12, TX12 MKII, Zorro) HAVE NOT been flight/field tested.** They compile cleanly with size-optimized profiles, but **require careful bench testing before any actual vehicle operation!**
> - Always perform a complete backup of your SD card and existing radio firmware prior to flashing. Remove propellers or disconnect motor power on RC cars during initial testing!

---

## 🚀 Overview & Key Advantages

This custom EdgeTX firmware turns your RadioMaster transmitter into a **bidirectional, zero-latency CRSF transceiver** over a single standard USB-C cable:

1. **Direct PC Control (100–250 Hz RX):**
   Your PC (running **RCSIM-GCS** or custom control software) transmits standard CRSF frames (Channels 1–16) directly over the USB-C Virtual COM Port (VCP) into the radio's mixer and RF module.
2. **Full-Duplex Telemetry Mirroring (TX back to PC):**
   Live telemetry packets transmitted from your RC vehicle (battery voltage/current/capacity, link statistics/RSSI/LQ/SNR, GPS position/speed/heading, and onboard IMU/attitude) are mirrored in real time and sent back through the USB-C cable directly to the PC.
3. **Zero Extra Hardware on your Desk (Zero Dongles):**
   No external Nomad modules, FTDI adapters, Arduino/RP2350 bridges, or DSC audio cables needed. A single factory USB-C cable plugged into your transmitter handles both high-speed control transmission and return telemetry!
4. **Hardware Force Feedback (FFB) & GCS Dashboard:**
   Telemetry streaming into RCSIM-GCS drives live cockpit dashboard gauges and provides physical vibration/traction loss effects on your Force Feedback sim wheel.

---

## 📦 Supported Radios & Binaries

| File | Supported Radio | Status | Notes |
|---|---|---|---|
| [`EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin) | **RadioMaster MT12** | Bench Tested (OK) | Surface radio (cars/boats) |
| [`EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin) | **RadioMaster Pocket** | Bench Tested (OK) | 512KB Flash optimized |
| [`EdgeTX_v2.10_Boxer_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Boxer_CRSF_VCP_FullDuplex.bin) | **RadioMaster Boxer** | ⚠️ Experimental (Untested) | Full Flash build |
| [`EdgeTX_v2.10_TX16S_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_TX16S_CRSF_VCP_FullDuplex.bin) | **RadioMaster TX16S / TX16S MKII** | ⚠️ Experimental (Untested) | Color LCD target |
| [`EdgeTX_v2.10_TX12MK2_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_TX12MK2_CRSF_VCP_FullDuplex.bin) | **RadioMaster TX12 MKII** | ⚠️ Experimental (Untested) | 512KB Flash optimized |
| [`EdgeTX_v2.10_TX12_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_TX12_CRSF_VCP_FullDuplex.bin) | **RadioMaster TX12 (V1)** | ⚠️ Experimental (Untested) | 512KB Flash optimized |
| [`EdgeTX_v2.10_Zorro_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Zorro_CRSF_VCP_FullDuplex.bin) | **RadioMaster Zorro** | ⚠️ Experimental (Untested) | 512KB Flash optimized |

### Additional Tools
- [`crsf_vcp_duplex_test.py`](./crsf_vcp_duplex_test.py) — Standalone Python verification script for bidirectional control (100 Hz) + live telemetry decoding.
- [`crsf_vcp_test.py`](./crsf_vcp_test.py) — Standalone simplex control test script (channels only).
- [`legacy_v1_simplex/`](./legacy_v1_simplex/) — Baseline PR #7630 builds (control only, no telemetry mirror).
- [`README_PL.md`](./README_PL.md) — Polska wersja dokumentacji.

---

## 🛠️ Step 1: Flashing Firmware to the Radio

The safest and most reliable flashing method is using the built-in EdgeTX Bootloader:

1. **Access the SD Card:**
   - Turn on your radio, connect it to the PC via USB-C, and choose **USB Storage (SD)**.
   - Alternatively, remove the MicroSD card from the radio and insert it into a PC card reader.
2. **Copy the Firmware:**
   - Navigate to the `FIRMWARE/` folder on the MicroSD card root.
   - Copy the appropriate `.bin` file matching your radio model into the `FIRMWARE/` folder.
3. **Eject & Enter Bootloader:**
   - Safely eject the SD card / disconnect the USB cable.
   - Power off the radio.
   - Hold both horizontal trim buttons inward (towards the power button) and press the **Power** button to boot into the **EdgeTX Bootloader**.
4. **Flash:**
   - Select **Write Firmware** on the radio screen.
   - Select the copied `.bin` file and confirm by holding Enter.
   - Once the progress bar reaches 100%, select **Exit** to reboot into normal EdgeTX mode.

---

## ⚙️ Step 2: EdgeTX Menu Configuration

Configure your radio model to accept trainer data from the USB Virtual COM Port:

### 1. System Setup (`SYS`)
*   **Radio Setup (`SYS` -> `Radio Setup`):**
    - Set `USB Mode` to **VCP** (or **Ask**, and select *VCP / Serial* whenever you plug in the USB cable).
*   **Hardware Configuration (`SYS` -> `Hardware`):**
    - Scroll to the `USB-VCP` (or `VCP`) port setting and change it to **CRSF Trainer**.

### 2. Model Setup (`MDL`)
*   **Trainer Mode (`MDL` -> `Model Setup`):**
    - Scroll down to the **Trainer** section:
      - `Mode`: **CRSF**
      - `Channels`: **CH1 - CH16**
*   **Trainer Function Assignment (`MDL` -> `Special Functions`):**
    - Add a new Special Function:
      - `Switch`: `ON` (or assign a toggle switch such as `SA` / `SB` to activate trainer control)
      - `Action`: **Trainer**
      - `Value`: **Sticks** (or **Axis**)
      - `Enable`: Checked (`ON`)

---

## 🧪 Step 3: Verification & Diagnostics

To verify both control input and return telemetry before launching RCSIM-GCS:

1. **Power on the RC vehicle** (ensure the receiver binds to the radio and starts sending telemetry).
2. Connect the radio to your PC via USB-C.
3. Open a terminal (PowerShell / Command Prompt) and run:
   ```bash
   python crsf_vcp_duplex_test.py
   ```
   *(Optional: specify port manually with `--port COM3` or `--baud 115200`)*
4. **Expected Output:**
   ```text
   ======================================================================
     CRSF USB-VCP Full-Duplex Test
   ======================================================================
   Otwarto port COM5 (115200 bps).
   [TX] Pętla 100 Hz uruchomiona.
   [TX] Wysłano 100 ramek sterujących (100 Hz)...
      <-- TELEMETRIA: [LINK STATS] RSSI: -46 dBm | LQ: 100% | SNR: 11 dB | RF Mode: 4
      <-- TELEMETRIA: [BATTERY] 8.24V | 1.35A | 412 mAh | 82%
      <-- TELEMETRIA: [GPS] 52.2297°N, 21.0122°E | Speed: 18.4 km/h | Sats: 14
   ```
5. On the radio's **Channel Monitor** screen, observe channels 1–4 smoothly sweeping based on test frames generated from the PC!

---

## 🏎️ Step 4: Integration with RCSIM-GCS

1. Launch **RCSIM-GCS** on your PC.
2. Go to the **Cockpit / Connection** panel:
   - Select your radio's Virtual COM Port (e.g., `COM5` - STMicroelectronics Virtual COM Port).
   - Baudrate: `115200 bps`.
   - Protocol: `CRSF Direct (USB-VCP)`.
3. Map your sim-racing steering wheel, pedals (throttle/brake), and handbrake to the desired channels in the **Input Configuration** tab.
4. Armed driving with real-time FFB telemetry feedback is now active!

---

## 📜 Technical Details, Source Code & GPLv3 Licensing

- **EdgeTX Version:** Based on EdgeTX 2.10 development tree, PR **#7630** (`feat/crsf-trainer-over-usb-vcp`, commit `e5784ee5`).
- **Telemetry Patch:** Integrates `telemetrySetMirrorCb` hook routing incoming CRSF telemetry directly to the USB-VCP TX endpoint buffer.
- **Source Code (GPLv3 Compliance):** In strict accordance with the GNU General Public License v3, the complete source code modifications applied to the upstream EdgeTX codebase are provided in this repository as a standard patch file: [`edgetx_crsf_vcp_full_duplex.patch`](./edgetx_crsf_vcp_full_duplex.patch).
- **Upstream Source Code Reference:** EdgeTX upstream repository is hosted at [EdgeTX/edgetx](https://github.com/EdgeTX/edgetx).
- **License:** GNU General Public License v3.0 (GPLv3). Distributed without warranty of any kind.
