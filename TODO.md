# TODO: Firmware Subsystem (Tiers 1, 2, 6)

## 🛠️ Do Naprawy / Stabilność
- [ ] **SBUS Jitter (Tier 6)**: Zaimplementować filtrację Median Filter dla odczytów SBUS w Watchdogu, aby wyeliminować drgania serw.
- [ ] **I2C Bus Collision**: Usprawnić obsługę błędów magistrali I2C przy jednoczesnym dostępie ESP32 i RPi.

## 🏗️ Architektura i Rozwój
- [ ] **WiFi Config Portal**: Dodać tryb AP z interfejsem WWW do konfiguracji SSID/Password bez reflashowania.
- [ ] **OTA Updates**: Wdrożyć system aktualizacji Over-The-Air dla ESP32.

## 📈 Innowacje (V38.xx)
- [ ] **Telemetry Passthrough**: Przesyłanie telemetrii z czujników I2C bezpośrednio przez ESP32 do GCS (bypass RPi dla niskich latencji).
