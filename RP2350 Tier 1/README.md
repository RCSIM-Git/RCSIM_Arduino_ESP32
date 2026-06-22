# RCSIM - RP2350 USB to PPM Bridge (Tier 1)

Firmware and technical specification for the **Seeed Studio XIAO RP2350** microcontroller acting as a USB-to-serial to PPM (Pulse Position Modulation) bridge. This mode is used in the RCSIM system for direct vehicle control using the trainer port of a transmitter (e.g., RadioMaster MT12) connected to the GCS station.

## Technical Specification

- **Microcontroller:** Seeed Studio XIAO RP2350
- **PPM Signal Output:** Pin D8 (GPIO 2) — 5V-tolerant on the XIAO board
- **Channel Count:** 8
- **PPM Frame Length:** 22.5 ms (22500 us)
- **Sync Pulse:** 300 us
- **Baudrate:** 115200 bps
- **Data Format:** Text CSV: `ch1,ch2,ch3,ch4,ch5,ch6,ch7,ch8\n` (800 - 2200 us, neutral 1500 us)

## Dual-Core Architecture

To eliminate PPM signal jitter caused by I/O operations, the firmware utilizes both cores of the RP2350 microcontroller:
- **Core 0 (Main Core):** Handles receiving CSV packets over USB CDC, validating, parsing, and managing Failsafe / E-STOP events.
- **Core 1 (Auxiliary Core):** Dedicated solely to high-precision PPM generation using cycle-accurate delay loops with system interrupts disabled. This prevents timing jitter.

## Safety & Failsafe

- **Hardware Signal Kill (Failsafe):** If the microcontroller does not receive a valid control frame within **500 ms**, it automatically enters Failsafe mode. The PPM output pin is configured as `Hi-Z` / `INPUT` mode, causing the transmitter to detect a lost signal ("Signal Lost") and immediately stop the vehicle.
- **Built-in commands:**
  - `ESTOP\n` — Immediate cut-off of the PPM signal (Emergency Stop).
  - `ARM\n` — Clear E-STOP and enable PPM transmission.

## Setup Guide (PlatformIO)

To compile the firmware, create a PlatformIO project with the following `platformio.ini`:

```ini
[env:seeed_xiao_rp2350]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = seeed_xiao_rp2350
framework = arduino
board_build.core = earlephilhower
monitor_speed = 115200
```

Place the `main.cpp` file inside the `src/` directory and upload it to the board.
