# RCSIM - Serial to PPM Converter for Arduino (v3.0 - High Stability)

This project contains the communication bridge (Bridge) firmware for the Arduino platform (e.g., Nano/Uno with ATmega328P microcontroller), converting serial commands from a Ground Control Station (GCS) computer into a standard PPM (Pulse Position Modulation) signal with a built-in hardware safety switch (Signal Kill).

## Main Features

1. **Serial-to-PPM Conversion (8 channels):**
   * Receives control data from the USB serial port (Baudrate: `115200 bps`) in textual format: `"ch1,ch2,...,ch8\n"`.
   * Processes and encodes microsecond values (range: 1000 - 2000 us) into a composite positive PPM signal, output on pin `PPM_OUTPUT_PIN` (D10).
   * PPM generation is powered by stable hardware interrupts (Timer1 for AVR).

2. **Critical Failsafe (Physical Signal Cutoff / Signal Kill):**
   * If valid control frames from the host PC are missing for over `500 ms` (e.g., GCS software crash or USB cable disconnect), the board enters failsafe mode.
   * Instead of sending neutral servo values, this module implements **Signal Kill**: it disables Timer1 interrupts and switches the output pin `PPM_OUTPUT_PIN` to high-impedance mode (`INPUT`).
   * The RC transmitter (e.g., RadioMaster MT12) connected via the trainer port instantly detects the loss of PPM signal and triggers its own internal failsafe actions (e.g., motor cutoff).

3. **Hardware Emergency Stop (E-STOP & ARM):**
   * The board responds to out-of-band text commands sent from the GCS:
     * `ESTOP` - Immediately stops PPM generation (physically disabling the output pin). The system ignores any subsequent control packets until re-armed.
     * `ARM` - Restores control output after clearing an emergency stop condition.

4. **LED State Indicators:**
   * **Disconnected / Failsafe / E-Stop:** The onboard LED (`LED_PIN`) is completely turned off.
   * **Normal Operation:** The LED blinks rapidly (~5 Hz), confirming active, valid data throughput and armed system status.

## Hardware Requirements & Libraries

* **Microcontroller:** Arduino Nano / Uno (ATmega328P).
* **Library:** `PPMEncoder` (responsible for hardware-based PPM generation using Timer1).
* **Wiring:** Pin D10 (PPM Output) -> Trainer port input (3.5mm jack on the RC transmitter).

## Usage Instructions

1. Upload the `Arduino.ino` sketch to your Arduino board using the Arduino IDE.
2. Connect the Arduino to your PC via a USB cable.
3. In the Ground Control Station (GCS) application, select **Arduino (Serial)** connection mode, choose the correct COM port, and set the baudrate to **115200 bps**.
4. Connect pin D10 and Ground (GND) from the Arduino to the transmitter's PPM trainer port.
