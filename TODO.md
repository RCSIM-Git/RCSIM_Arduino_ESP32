# TODO: Firmware Subsystem (Tiers 1, 2, 6)

## 🛠️ Do Naprawy / Stabilność
- [x] **Arduino E-Stop Support (Tier 1)**: Dodano obsługę komend ESTOP i ARM w konwerterze PPM w celu fizycznego odcinania sygnału podczas awaryjnego zatrzymania klawiszem SPACJA.
- [ ] **SBUS Jitter (Tier 6)**: Zaimplementować filtrację Median Filter dla odczytów SBUS w Watchdogu, aby wyeliminować drgania serw.
- [x] **I2C Bus Collision**: Wdrożono autoodzyskiwanie magistrali I2C (Bus Recovery) i limit czasu (Timeout) chroniące przed zawieszeniem pętli głównej.

## 🏗️ Architektura i Rozwój
- [ ] **WiFi Config Portal**: Dodać tryb AP z interfejsem WWW do konfiguracji SSID/Password bez reflashowania.
- [ ] **OTA Updates**: Wdrożyć system aktualizacji Over-The-Air dla ESP32.

## 📈 Innowacje (V38.xx)
- [ ] **Telemetry Passthrough**: Przesyłanie telemetrii z czujników I2C bezpośrednio przez ESP32 do GCS (bypass RPi dla niskich latencji).
