# Build Instructions — ExpressLRS RadioMaster ER Series Custom Firmware (GPS + IMU)

> These instructions document how to reproduce the `.bin` and `.bin.gz` firmware binaries
> distributed for the RadioMaster ER Series receivers (ER4, ER5A/C, ER6, ER8) from the corresponding
> source code. Together with the complete source tree, they satisfy the Corresponding Source and build
> instructions requirements of the GNU General Public License v3.0 (GPL-3.0 §6).

---

## 1. Prerequisites

### Build System & Toolchain
ExpressLRS builds are managed via **PlatformIO** (using Python 3 and PlatformIO Core CLI).

- **Python:** 3.10+
- **PlatformIO Core:** `pip install platformio`
- **Node.js:** v18+ (needed for building receiver WebUI frontend assets if modified)
- **Git**

Verify installations:
```bash
pio --version
python --version
node --version
```

---

## 2. Source Code Repository

Clone the full corresponding source code repository:
```bash
git clone https://github.com/RCSIM-Git/expresslrs-er-series-telemetry.git
cd expresslrs-er-series-telemetry
git checkout de876c0286c2788b4e02fac267078c374501f0ec
```

All 9 receiver target binaries in this release batch were compiled from this single source state.

---

## 3. WebUI Frontend Compilation (Optional)

If modifying `src/html/`:
```bash
cd src/html
npm install
npm run build
cd ../..
```

---

## 4. Compiling Firmware Binaries

PlatformIO projects in ExpressLRS are built from the `src/` directory.

### ESP8285 Targets (ER4, ER5A/C V1, ER5A/C V2)
Compile the unified base binary for ESP8285 2.4GHz RX:
```bash
cd src
pio run -e Unified_ESP8285_2400_RX_via_WIFI
```
The compiled outputs will be located at:
- `src/.pio/build/Unified_ESP8285_2400_RX_via_WIFI/firmware.bin`
- `src/.pio/build/Unified_ESP8285_2400_RX_via_WIFI/firmware.bin.gz`

### ESP32 Targets (ER6, ER6G, ER6GV, ER8, ER8G, ER8GV)
Compile the unified base binary for ESP32 2.4GHz RX:
```bash
cd src
pio run -e Unified_ESP32_2400_RX_via_WIFI
```
The compiled outputs will be located at:
- `src/.pio/build/Unified_ESP32_2400_RX_via_WIFI/firmware.bin`
- `src/.pio/build/Unified_ESP32_2400_RX_via_WIFI/firmware.bin.gz`

---

## 5. Applying Unified Target Configuration Layouts

ExpressLRS Unified binaries require appending receiver-specific hardware layout metadata (pin assignments, RF power levels, etc.) using `UnifiedConfiguration.py` from `src/python/`.

Run the automated build & release packager:
```bash
python build_gps_release.py
```
This script injects the target layout JSON (e.g. for RadioMaster ER4, ER5Cv2, ER6, ER8) and compresses the finalized binaries into `.bin` and `.bin.gz` formats ready for OTA Wi-Fi WebUI flashing or direct FTDI UART upload via `esptool.py`.
