# TODO: Firmware Subsystem (Tiers 1, 2, 6)

## 🛠️ Do Naprawy / Stabilność
- [x] **RP2350 USB-to-PPM (Tier 1)**: Naprawiono obsługę polecenia ARM w `main.cpp`, czyszczenie bufora USB CDC w Pythonie oraz domyślną inicjalizację portów COM w GCS.
- [x] **Arduino E-Stop Support (Tier 1)**: Dodano obsługę komend ESTOP i ARM w konwerterze PPM w celu fizycznego odcinania sygnału podczas awaryjnego zatrzymania klawiszem SPACJA.
- [ ] **SBUS Jitter (Tier 6)**: Zaimplementować filtrację Median Filter dla odczytów SBUS w Watchdogu, aby wyeliminować drgania serw.
- [x] **I2C Bus Collision**: Wdrożono autoodzyskiwanie magistrali I2C (Bus Recovery) i limit czasu (Timeout) chroniące przed zawieszeniem pętli głównej.
- [x] **ExpressLRS ER5C V2 I2C-IMU Mod & Video Guide (Tier 3)**: Zsynchronizowano dedykowany poradnik wideo (YouTube: `CIJ9e5cBtAE`), instrukcję pinoutu I2C (CH4/CH5), wgrywania OTA/FTDI, konfiguracji WebUI oraz telemetrii CRSF 0x86 na stronie internetowej i w Kompendium Wiedzy.
- [x] **Direct USB & Transmitter Compatibility Matrix (Tier 3)**: Opracowano i wdrożono pełną dokumentację trybu Direct USB (eliminacja adapterów, pełna moc anteny), matrycę pinoutów i ustawień dla wszystkich popularnych nadajników (Nomad, Ranger, BetaFPV, Happymodel, TBS, aparatur z wbudowanym ELRS), pułapkę Backpacka na ESP32 (3/1 vs 16/17), przełączniki DIP, procedurę revertu do radia oraz krytyczne zasady zasilania USB Fast-Charge (1W / 3A).

## 🏗️ Architektura i Rozwój
- [ ] **WiFi Config Portal**: Dodać tryb AP z interfejsem WWW do konfiguracji SSID/Password bez reflashowania.
- [ ] **OTA Updates**: Wdrożyć system aktualizacji Over-The-Air dla ESP32.

## 📈 Innowacje (V38.xx)
- [ ] **Telemetry Passthrough**: Przesyłanie telemetrii z czujników I2C bezpośrednio przez ESP32 do GCS (bypass RPi dla niskich latencji).
