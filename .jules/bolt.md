## 2026-06-14 - Avoid floating point math on AVR microcontrollers
**Learning:** 8-bit AVR microcontrollers (like Arduino Uno/Nano) do not have a hardware Floating Point Unit (FPU). Any operations using `float` or `double` are implemented via slow software emulation, which wastes significant CPU cycles and inflates binary size. This was observed in a low-pass filter using a `float filter_beta`.
**Action:** Always use integer math (multiplication + division) or bitshifting approximations instead of floating point math for performance-critical code on AVR targets.
