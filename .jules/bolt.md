## 2024-06-11 - Floating Point Math on AVR
**Learning:** 8-bit AVR microcontrollers (like the Arduino Nano used in this project) lack hardware floating-point units. Using `float` operations (like `float filter_beta = 0.3`) results in slow software emulation which wastes CPU cycles.
**Action:** Always replace basic `float` arithmetic with integer approximations using fixed-point math, multiplication, and division (or bitshifting when applicable). For example, replacing `x * 0.3` with `(x * 3) / 10`.
