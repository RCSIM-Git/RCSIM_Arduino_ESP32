# RCSIM - Hardware Watchdog and Muxer for ESP32 (RCSIM HAT V1.0)

A safety co-processor (Supervisor Controller) built around the ESP32 microcontroller, responsible for system health monitoring, crash detection of the primary single-board computer (Raspberry Pi 5), and manual control takeover (Manual Override).

## Main Features

1. **Heartbeat Monitoring (Watchdog):**
   * Actively monitors heartbeat transitions from the Raspberry Pi 5 on `PIN_HEARTBEAT` (GPIO 4) using hardware interrupts.
   * If the pin state remains static for more than `1000 ms`, the system flags a Linux crash/freeze and triggers failsafe mode.

2. **Manual Override (Native SBUS Decoding):**
   * Reads raw SBUS data from the RC receiver on `PIN_SBUS_RX` (GPIO 15) using hardware UART2.
   * Leverages the ESP32's built-in serial signal inversion capability, decoding the SBUS stream directly **without needing an external transistor inverter circuit**.
   * Flipping the override channel switch (default: Channel 5 > 1600) immediately passes vehicle control back to the operator.

3. **Double-Failsafe (Signal Loss Protection):**
   * The system monitors both the Raspberry Pi 5 status and the RC connection health.
   * If the operator switches to manual mode but the RC transmitter loses connection (failsafe flag set by receiver), the coprocessor immediately forces the system into `STATE_FAILSAFE`, stopping the vehicle.

4. **I2C Bus Protection (No PCA9685 Spamming):**
   * Utilizes a finite state machine (`SystemState`).
   * Instead of sending I2C commands continuously in a loop (which could clog the shared bus or inject electrical noise), failsafe commands (`applySafeState`) and state transitions are transmitted to the PCA9685 **exactly once** during the transition.
   * Continuous PWM telemetry updating occurs only in manual mode (`STATE_MANUAL_OVR`) as stick positions actively change.

5. **Muxer Override Feedback (`PIN_OVR_STATUS`):**
   * Pin `PIN_OVR_STATUS` (GPIO 5) indicates the active bus owner to the Raspberry Pi 5:
     * `HIGH` - Normal mode, Raspberry Pi controls the vehicle.
     * `LOW` - Override mode (Manual/Failsafe), instructing the RPi5 to release I2C control and adjust its behavior (e.g., stop AI navigation/logging).

## Library Requirements

Compiling this sketch requires the following libraries:
* **bolderflight/Bolder Flight Systems SBUS** (available via PlatformIO/Arduino Library Manager)
* **Adafruit PWM Servo Driver Library** (for PCA9685 communication)
