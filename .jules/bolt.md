## 2024-05-24 - Initial Review
**Learning:** Found an SBUS filtering issue identified as a performance task in TODO.md.
**Action:** Implement a median filter for SBUS inputs in `ESP32 Watchdog.ino`.

## 2024-05-24 - snprintf Return Value Optimization
**Learning:** Calling `strlen()` on a buffer immediately after populating it with `snprintf()` forces a redundant O(N) string traversal. In high-frequency operations like IMU telemetry loops (20Hz+ over UDP), this causes unnecessary CPU overhead.
**Action:** Always capture and use the integer return value of `snprintf()` (which represents the number of characters written) to avoid calling `strlen()` entirely.
