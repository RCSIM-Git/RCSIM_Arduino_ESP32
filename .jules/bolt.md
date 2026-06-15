## 2026-06-12 - [8-Bit AVR Float Anti-pattern]
**Learning:** [Avoid using floating-point (`float`) arithmetic on 8-bit AVR microcontrollers (e.g., Arduino Nano). They lack a hardware FPU, so float operations require slow software emulation, creating a performance bottleneck.]
**Action:** [Use integer math or bitshifting approximations instead of floats on AVR targets. E.g., replace `val * 0.3` with `(val * 3) / 10`.]
