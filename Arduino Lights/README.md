# RCSIM - Smart RC Light Controller for Arduino (v1.0)

This project is the firmware for a dedicated LED light controller for RC vehicles, built on an Arduino microcontroller (e.g., Nano/Uno with the ATmega328P chip). The system automatically decodes PWM signals from the RC receiver and adjusts the vehicle's light states (headlights, brakes, reverse, turn signals).

## Key Features

1. **Two Operating Modes (Auto-detection):**
   * **Full Mode (Multi-channel):** CH1 (Steering), CH2 (Throttle), and CH3 (Auxiliary switch) signals are connected. The lights react fully dynamically to the vehicle's behavior.
   * **Single-Channel Mode (Muxed Fallback):** Only the CH3 channel is connected. This allows basic control in simplified installations: CH3 controls the turn signals (low position = left, high = right, middle = off), while the headlights and tail lights remain constantly on.

2. **Non-blocking Interrupt-driven Reading (PCINT):**
   * Pulse width measurement (PWM) from the receiver is handled via hardware Pin Change Interrupts (`PCINT0` registers on pins D8, D9, D10).
   * This ensures the microcontroller does not block the main loop with slow `pulseIn()` calls, guaranteeing smooth execution and instantaneous light reactions.

3. **Voltage Protection (Active-Low Grounding):**
   * LEDs are driven active-low.
   * To turn an LED off, the microcontroller pin is not set `HIGH` (which could cause reverse current from the 5V Arduino to the receiver/ESC's 3.3V rail); instead, it transitions to a **high-impedance state (`INPUT`)**. This protects the receiver and ESC electronics.

4. **Light Logic in Full Mode:**
   * **Front Headlights (D2) & Aux Lights (D7):** Controlled by a 3-position switch on CH3 (down = off, middle = headlights ON, up = all ON).
   * **Brake Lights (D3):** Shine at full brightness when braking or at throttle neutral (acting as tail lights). They turn off when driving forward.
   * **Reverse Lights (D4):** Turn on automatically when reverse throttle input is detected.
   * **Automatic Turn Signals (D5 / D6):** Triggered by steering wheel deflection (CH1). Features a time hysteresis (`turn_signal_delay_threshold = 300 ms`) to prevent turn signals from flashing during quick steering corrections or sweeping turns.

5. **Failsafe Mode:**
   * If no receiver signals are detected for over `500 ms`, the main headlights turn off and **hazard lights** (simultaneous flashing of left and right turn signals) activate, indicating a loss of signal or wiring failure.

## Controller Pinout (Arduino Nano)

| Function | Arduino Pin | Connection Description |
|---|---|---|
| **CH1 Input** | **D8** | PWM Steering signal from RC receiver |
| **CH2 Input** | **D9** | PWM Throttle/Brake signal from RC receiver |
| **CH3 Input** | **D10** | PWM Aux signal from RC receiver |
| **Front Headlights** | **D2** | Front LED ground control |
| **Brake / Tail Lights** | **D3** | Rear red LED ground control |
| **Reverse Lights** | **D4** | Reverse LED ground control |
| **Left Turn Signal** | **D5** | Left turn indicator LED ground control |
| **Right Turn Signal**| **D6** | Right turn indicator LED ground control |
| **Roof / Aux Lights** | **D7** | Additional/roof LED ground control |

## Setup and Calibration

1. Flash the `RCSIM_Lights.ino` sketch to your Arduino Nano board using the Arduino IDE.
2. Connect inputs D8, D9, and D10 to the corresponding channels on your RC receiver (using Y-cables for shared signal and ground connections from the ESC/BEC).
3. Connect the LEDs via current-limiting resistors to the power supply (connect the LED cathodes to Arduino pins D2-D7).
4. Upon power-up, the system automatically calibrates the neutral points for steering and throttle (ensure transmitter sticks are at neutral for the first second after power-on).

```
+-------------------+
|   RC Receiver     |
|                   |
| CH1  CH2   CH3    |
+-|-----|-----|-----+
  |     |     |
[Y-Cab][Y-Cab]       
  /   \   /   \ 
 /     \ /     \        (Signal + GND)
[Servo] [ESC]   +------> To Arduino CH3 (D10 + GND)
  |      |               and Arduino power (5V)
  |      |
  |      +-------------> To Arduino CH2 (D9 Signal + GND)
  +--------------------> To Arduino CH1 (D8 Signal + GND)
```