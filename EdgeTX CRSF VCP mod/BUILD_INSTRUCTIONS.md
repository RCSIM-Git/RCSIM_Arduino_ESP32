# Build Instructions — EdgeTX CRSF VCP Full-Duplex Firmware

> These instructions document how to reproduce every `.bin` file distributed
> in this directory from the corresponding source code. Together with the
> source archive (or the prepared source tree), they satisfy the
> "scripts used to control compilation and installation" requirement of
> [GPL-2.0 §3](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).

---

## Prerequisites

### Toolchain

The binaries were built with the **ARM GCC** cross-compiler toolchain.
Record your exact version so recipients can reproduce equivalent output:

```
arm-none-eabi-gcc --version
```

**Tested with:** Version not recorded. The builds were produced with a standard `arm-none-eabi-gcc` toolchain on the EdgeTX Docker build environment.

> [!NOTE]
> GPL-2.0 does not require distributing the compiler itself (it falls under
> the "major components of the operating system" exception in §3), but
> documenting the version helps recipients reproduce equivalent builds.

### Build dependencies

- **CMake** ≥ 3.16
- **Python 3** (used by EdgeTX build scripts)
- **GNU Make** or **Ninja**
- **git** (to verify the source tree)

On Debian/Ubuntu:
```bash
sudo apt install cmake python3 ninja-build git
```

---

## Source Tree Preparation

If you have the source archive (`edgetx-crsf-vcp-fullduplex-source.tar.gz`):

```bash
tar xzf edgetx-crsf-vcp-fullduplex-source.tar.gz
cd edgetx-crsf-vcp-source
```

If you are preparing from scratch, use `prepare_source.sh`:
```bash
./prepare_source.sh
cd edgetx-crsf-vcp-source
```

> [!NOTE]
> The `feat/crsf-trainer-over-usb-vcp` branch (PR #7630) lives on
> [BelixRogner/edgetx](https://github.com/BelixRogner/edgetx), not on
> the upstream `EdgeTX/edgetx` repository.

### Verify the source state

```bash
# Should show exactly one commit on top of e5784ee5:
git log --oneline -3
```

---

## Build Commands — All Seven Targets

All builds share the same source tree. The only differences are the
`-DPCB` and `-DPCBREV` CMake variables and, for 512KB targets, the
feature-stripping options (`-DPXX1=OFF`, `-DPXX2=OFF`, etc.) which are
already applied in the patched `CMakeLists.txt` and `build-common.sh`.

The patch also applies **globally** to all targets:
- Aggressive size-optimisation flags in `radio/src/CMakeLists.txt`
  (`-fno-unwind-tables`, `-fno-asynchronous-unwind-tables`,
   `-fno-align-functions`, `-fno-align-jumps/loops/labels`)
- LTO for the bootloader (`radio/src/bootloader/CMakeLists.txt`)
- Bidirectional VCP + telemetry mirror (`radio/src/serial.cpp`)
- `__attribute__((used))` on `SystemInit`/`SystemClock_Config`/
  `SystemCoreClockUpdate` to prevent LTO stripping them
- Conditional FatFs features in bootloader context (`ffconf.h`)

### General build procedure

For each target, create a separate build directory and run CMake:

```bash
mkdir -p build-<TARGET> && cd build-<TARGET>
cmake -DCMAKE_BUILD_TYPE=Release <TARGET_OPTIONS> ../radio/src
make -j$(nproc) firmware
```

The firmware binary will be at: `build-<TARGET>/firmware.bin`

---

### 1. RadioMaster MT12

```bash
mkdir -p build-mt12 && cd build-mt12
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X7 \
      -DPCBREV=MT12 \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

**Output:** `build-mt12/firmware.bin` → `EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin`
**Status:** Bench tested (OK)

---

### 2. RadioMaster Pocket (512KB Flash)

```bash
mkdir -p build-pocket && cd build-pocket
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X7 \
      -DPCBREV=POCKET \
      -DHELI=NO \
      -DGHOST=NO \
      -DPXX1=OFF \
      -DPXX2=OFF \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

> **Note:** The `-DHELI=NO -DGHOST=NO -DPXX1=OFF -DPXX2=OFF` options
> are also applied in the patched `tools/build-common.sh` for the `pocket`
> target, and partially in `targets/taranis/CMakeLists.txt`. Specifying
> them explicitly on the command line ensures they take effect regardless
> of build entry point.

**Output:** `build-pocket/firmware.bin` → `EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin`
**Status:** Bench tested (OK)

---

### 3. RadioMaster Boxer

```bash
mkdir -p build-boxer && cd build-boxer
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X7 \
      -DPCBREV=BOXER \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

**Output:** `build-boxer/firmware.bin` → `EdgeTX_v2.10_Boxer_CRSF_VCP_FullDuplex.bin`
**Status:** ⚠️ Experimental (untested on hardware)

---

### 4. RadioMaster TX16S / TX16S MKII

```bash
mkdir -p build-tx16s && cd build-tx16s
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X10 \
      -DPCBREV=TX16S \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

**Output:** `build-tx16s/firmware.bin` → `EdgeTX_v2.10_TX16S_CRSF_VCP_FullDuplex.bin`
**Status:** ⚠️ Experimental (untested on hardware)

---

### 5. RadioMaster TX12 MKII (512KB Flash)

```bash
mkdir -p build-tx12mk2 && cd build-tx12mk2
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X7 \
      -DPCBREV=TX12MK2 \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

> **Note:** The patched `targets/taranis/CMakeLists.txt` already sets
> `PXX2=OFF PXX1=OFF HELI=NO GHOST=NO` for `PCBREV=TX12MK2`.

**Output:** `build-tx12mk2/firmware.bin` → `EdgeTX_v2.10_TX12MK2_CRSF_VCP_FullDuplex.bin`
**Status:** ⚠️ Experimental (untested on hardware)

---

### 6. RadioMaster TX12 V1 (512KB Flash)

```bash
mkdir -p build-tx12 && cd build-tx12
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X7 \
      -DPCBREV=TX12 \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

> **Note:** The patched `targets/taranis/CMakeLists.txt` already sets
> `PXX2=OFF PXX1=OFF HELI=NO GHOST=NO` for `PCBREV=TX12`.

**Output:** `build-tx12/firmware.bin` → `EdgeTX_v2.10_TX12_CRSF_VCP_FullDuplex.bin`
**Status:** ⚠️ Experimental (untested on hardware)

---

### 7. RadioMaster Zorro (512KB Flash)

```bash
mkdir -p build-zorro && cd build-zorro
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPCB=X7 \
      -DPCBREV=ZORRO \
      ../radio/src
make -j$(nproc) firmware
cd ..
```

> **Note:** The patched `targets/taranis/CMakeLists.txt` already sets
> `PXX2=OFF PXX1=OFF HELI=NO GHOST=NO` for `PCBREV=ZORRO`.

**Output:** `build-zorro/firmware.bin` → `EdgeTX_v2.10_Zorro_CRSF_VCP_FullDuplex.bin`
**Status:** ⚠️ Experimental (untested on hardware)

---

## Build All Targets (convenience script)

```bash
#!/usr/bin/env bash
set -euo pipefail

TARGETS=(
    "mt12:X7:MT12:"
    "pocket:X7:POCKET:-DHELI=NO -DGHOST=NO -DPXX1=OFF -DPXX2=OFF"
    "boxer:X7:BOXER:"
    "tx16s:X10:TX16S:"
    "tx12mk2:X7:TX12MK2:"
    "tx12:X7:TX12:"
    "zorro:X7:ZORRO:"
)

SRC_DIR="$(pwd)/radio/src"

for entry in "${TARGETS[@]}"; do
    IFS=':' read -r name pcb rev extra <<< "${entry}"
    build_dir="build-${name}"
    echo "=== Building ${name} (PCB=${pcb}, PCBREV=${rev}) ==="
    mkdir -p "${build_dir}"
    pushd "${build_dir}" > /dev/null
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DPCB="${pcb}" \
          -DPCBREV="${rev}" \
          ${extra} \
          "${SRC_DIR}"
    make -j$(nproc) firmware
    popd > /dev/null
    echo "=== ${name}: done → ${build_dir}/firmware.bin ==="
    echo ""
done

echo "All targets built successfully."
```

---

## Summary of Patch-Modified Files

| File | Nature of Change |
|---|---|
| `radio/src/CMakeLists.txt` | Global size-optimisation compiler flags |
| `radio/src/bootloader/CMakeLists.txt` | LTO for bootloader, extra size flags |
| `radio/src/serial.cpp` | Bidirectional VCP (TX+RX) + `telemetrySetMirrorCb` |
| `radio/src/targets/common/arm/stm32/f4/system_clock.c` | `__attribute__((used))` on `SystemClock_Config` |
| `radio/src/targets/common/arm/stm32/f4/system_stm32f4xx.c` | `__attribute__((used))` on `SystemInit`, `SystemCoreClockUpdate` |
| `radio/src/targets/taranis/CMakeLists.txt` | Disable PXX1/PXX2/HELI/GHOST on 512KB targets |
| `radio/src/thirdparty/FatFs/ffconf.h` | Conditional `FF_USE_MKFS`, `FF_USE_CHMOD`, `FF_FS_RPATH` in bootloader |
| `tools/build-common.sh` | Pocket target build options |

---

## License

EdgeTX is distributed under the
[GNU General Public License v2.0 (GPL-2.0)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).

The modifications in this source tree are also distributed under GPL-2.0,
in accordance with the terms of the original license.

This is not an official EdgeTX release.
