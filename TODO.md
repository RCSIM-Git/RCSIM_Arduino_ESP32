# TODO: Firmware Subsystem (Tiers 1, 2, 6)

## 🛠️ Do Naprawy / Stabilność
- [x] **RP2350 USB-to-PPM (Tier 1)**: Naprawiono obsługę polecenia ARM w `main.cpp`, czyszczenie bufora USB CDC w Pythonie oraz domyślną inicjalizację portów COM w GCS.
- [x] **Arduino E-Stop Support (Tier 1)**: Dodano obsługę komend ESTOP i ARM w konwerterze PPM w celu fizycznego odcinania sygnału podczas awaryjnego zatrzymania klawiszem SPACJA.
- [ ] **SBUS Jitter (Tier 6)**: Zaimplementować filtrację Median Filter dla odczytów SBUS w Watchdogu, aby wyeliminować drgania serw.
- [x] **I2C Bus Collision**: Wdrożono autoodzyskiwanie magistrali I2C (Bus Recovery) i limit czasu (Timeout) chroniące przed zawieszeniem pętli głównej.
- [x] **ExpressLRS ER5C V2 I2C-IMU Mod & Video Guide (Tier 3)**: Zsynchronizowano dedykowany poradnik wideo (YouTube: `CIJ9e5cBtAE`), instrukcję pinoutu I2C (CH4/CH5), wgrywania OTA/FTDI, konfiguracji WebUI oraz telemetrii CRSF 0x86 na stronie internetowej i w Kompendium Wiedzy.
- [x] **ExpressLRS ER Series GPS + IMU All-In-One Mod (Tier 3)**: Rozszerzono wsady z ER5C V2 na całą serię odbiorników RadioMaster ER (ER4, ER5A/C, ER6, ER6G, ER6GV, ER8, ER8G, ER8GV) z obsługą GPS (UART NMEA 9600-115200 bps z auto-baudrate), ochroną pinu PWM dla ESC, telemetrią IMU (CRSF 0x02, 0x03, 0x86), jednolitym patchem GPLv3 oraz kompletną dokumentacją PL/EN.
- [x] **Direct USB & Transmitter Compatibility Matrix (Tier 3)**: Opracowano i wdrożono pełną dokumentację trybu Direct USB (eliminacja adapterów, pełna moc anteny), matrycę pinoutów i ustawień dla wszystkich popularnych nadajników (Nomad, Ranger, BetaFPV, Happymodel, TBS), pułapkę Backpacka na ESP32 (3/1 vs 16/17), przełączniki DIP, procedurę revertu do radia oraz krytyczne zasady zasilania USB Fast-Charge (1W / 3A).
- [x] **ExpressLRS ER5C V2 Auto I2C Address (0x68 / 0x69) & Clone Support (Tier 3)**: Rozszerzono `devImu.cpp` i `mpu9250.cpp` o dynamiczne wykrywanie i przekazywanie adresu I2C (`0x68` i `0x69`, np. pin AD0 pływający na modułach SEN-MPU6050) do procedury inicjalizacji oraz dodano obsługę sygnatur WHO_AM_I popularnych klonów (0x72, 0x98). Zrekompilowano i zaktualizowano wsady `.bin` i `.bin.gz`.
- [x] **ExpressLRS ER5C V2 I2C Bus Recovery, 100kHz Clock & Realtime Diagnostics (Tier 3)**: Obniżono taktowanie programowego I2C z 400kHz do 100kHz (Standard Mode) w celu eliminacji problemów z czasem narastania sygnału na przewodach DuPont, dodano 9-taktową sekwencję zwalniania zawieszonej linii SDA (`I2C Bus Clear`), zastąpiono problematyczny Repeated-Start czystą sekwencją STOP oraz zaimplementowano raportowanie kodów błędów I2C i bajtów WHO_AM_I w ramce telemetrii CRSF 0x86 (AX, AY, GX, GY, GZ) przy braku inicjalizacji.
- [x] **EdgeTX CRSF USB-VCP Full-Duplex Mod (RadioMaster MT12 & Pocket)**: Przygotowano zmodyfikowane wsady EdgeTX 2.10 (PR #7630 + patch `telemetrySetMirrorCb`) umożliwiające bezpośrednie sterowanie kanałami 1-16 (100 Hz) oraz zwrot pełnej telemetrii (Link Stats, Bateria, GPS, IMU) przez pojedynczy kabel USB-C bez zewnętrznych mostków. Dodano skrypty diagnostyczne w Pythonie oraz pełną dokumentację PL i ENG.

## 🏗️ Architektura i Rozwój
- [x] **ESP32 Tier 2 Pro (CRSF Multi-Link Hub)**: Opracowano zaawansowany pokładowy hub sterowania łączący protokół CRSF (Crossfire / ELRS z sumą CRC8 DVB-S2), wielowarstwową komunikację (ESP-NOW z opóźnieniem 1-2 ms, Hardware Serial, UDP/LTE), sterownik PCA9685 (I2C 400kHz z autoodzyskiwaniem), sensorykę IMU (DLPF), GPS (NMEA) i ADC1 oraz sprzętowy Fail-Safe oparty na Dual-Core FreeRTOS.
- [ ] **WiFi Config Portal**: Dodać tryb AP z interfejsem WWW do konfiguracji SSID/Password bez reflashowania.
- [ ] **OTA Updates**: Wdrożyć system aktualizacji Over-The-Air dla ESP32.

## 📈 Innowacje (V38.xx)
- [ ] **Telemetry Passthrough**: Przesyłanie telemetrii z czujników I2C bezpośrednio przez ESP32 do GCS (bypass RPi dla niskich latencji).
