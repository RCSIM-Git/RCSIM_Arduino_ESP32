# Plan Implementacji: ESP32 Tier 2 Pro (CRSF Multi-Link Hub)

## 📌 Cel i Zakres
Celem jest stworzenie zaawansowanego oprogramowania układowego (Firmware) dla mikrokontrolera **ESP32**, które łączy:
1. Standard modelarski **CRSF (Crossfire / ELRS)** dla sterowania i telemetrii.
2. Modułowy interfejs transmisyjny (**ESP-NOW**, opcjonalnie **LoRa SX1262/SX1280** oraz **GSM/LTE**).
3. Peryferia pojazdu: sterownik serw/ESC **PCA9685** (I2C), czujnik bezwładnościowy **IMU** (MPU6050/MPU9250/BNO085/BMX160), **GPS** (UART NMEA), dzielnik napięcia **ADC1** oraz opcjonalnie OSD **Walksnail MSP**.
4. Niezawodny sprzętowy **Fail-Safe** i architekturę **FreeRTOS Dual-Core** zapobiegającą blokowaniu pętli sterowania.

---

## 🏗️ Architektura Systemu

### Przepływ Danych i Podział na Rdzenie (FreeRTOS)
```
  [WARSTWA FIZYCZNA: ESP-NOW / LoRa / LTE / Serial]
                       │ (Surowe ramki CRSF)
                       ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ CORE 0: PROTOKÓŁ & TELEKOMUNIKACJA                          │
 │  • Odbiór bajtów (ESP-NOW Callback / UART RX)               │
 │  • CRSF Frame Parser FSM (CRC8 DVB-S2)                      │
 │  • Dekodowanie Type 0x16: RC Channels Packed (16 kanałów)   │
 │  • Watchdog Łączności (Fail-Safe >100ms)                    │
 │  • Formater Telemetrii Zwrotnej CRSF:                       │
 │     - 0x02 GPS                                              │
 │     - 0x08 Battery                                          │
 │     - 0x1E Attitude (IMU)                                   │
 │     - 0x14 Link Statistics (RSSI/LQI)                       │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Queue / Mutex
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │ CORE 1: SPRZĘT & SENSORYKA W CZASIE RZECZYWISTYM            │
 │  • Zapis PWM PCA9685 (0x40): Odświeżanie serw i gazu        │
 │  • Odczyt nieblokujący IMU (0x68): Pitch, Roll, Yaw         │
 │  • Odczyt GPS (UART RX1): TinyGPS++                         │
 │  • Pomiar ADC1 (GPIO 33): Filtr EMA baterii                 │
 │  • Sprzętowy Task Watchdog (WDT 2s)                         │
 └─────────────────────────────────────────────────────────────┘
```

---

## 📦 Struktura Katalogów Projektu
Projekt zostanie umieszczony w dedykowanym katalogu, zachowując nienaruszone dotychczasowe wersje (V1, V2, V3):
`c:\Users\Mateusz\Desktop\RCSIM27.04monacoSLAM\RCSIM_PC\pc_app\ESP32Arduino\RCSIM_Arduino_ESP32\ESP32 Tier 2\ESP32\ESP32V4_CRSF_MultiLink\`
- `ESP32V4_CRSF_MultiLink.ino` – Główny szkic integrujący FreeRTOS, pętle i setup
- `CRSFParser.h` – Lekki automat stanów (FSM) dekodera i enkodera CRSF (CRC8 DVB-S2)
- `CRSFTransport.h` – Uniwersalny interfejs transportowy (ESP-NOW / LoRa / Serial / GSM)
- `PCA9685Manager.h` – Obsługa szyny I2C, kalibracji i przeliczania 11-bit CRSF na impulsy PWM
- `SensorsManager.h` – Odczyt IMU, GPS, ADC oraz generowanie struktur telemetrycznych
- `README_PL.md` i `README.md` – Instrukcje połączeń, konfiguracji i wgrania

---

## ⚙️ Wdrożenie Krok po Kroku

### Krok 1: Silnik CRSF i CRC8 DVB-S2 (`CRSFParser.h`)
- Zaimplementowanie automatu stanów:
  - `SYNC (0xC8 / 0xEE)` $\rightarrow$ `LEN` $\rightarrow$ `TYPE` $\rightarrow$ `PAYLOAD` $\rightarrow$ `CRC8`
- Funkcja sprawdzania i generowania sumy kontrolnej CRC8 wielomianem `0xD5`.
- Dekoder `0x16 RC_CHANNELS_PACKED`:
  - 22 bajty rozpakowywane na 16 wartości 11-bitowych (172 = 988 µs, 992 = 1500 µs, 1811 = 2012 µs).
  - Wykrywanie stanu uzbrojenia modelu (Standard ELRS AUX1 / Kanał 5: >1500 µs = ARMED, <1300 µs = DISARMED).
- Enkoder ramek telemetrii:
  - `0x02 GPS`: Lat, Lon, Groundspeed, Heading, Alt, Satellites.
  - `0x08 Battery`: Voltage (0.1V), Current (0.1A), Capacity (mAh), Remaining (%).
  - `0x1E Attitude`: Pitch, Roll, Yaw (skalowane do $10^{-4}$ rad).
  - `0x14 Link Statistics`: RSSI (dBm), Link Quality (0-100%).

### Krok 2: Warstwa Transmisji Radiowej (`CRSFTransport.h`)
- Zdefiniowanie klasy bazowej `CRSFTransport`.
- **Wariant 1: ESP-NOW (Domyślny / Zero-Latency):**
  - Odbiór w `esp_now_register_recv_cb` z natychmiastowym przekazaniem bajtów do kolejki FreeRTOS.
  - Odsyłanie telemetrii przez `esp_now_send`.
- **Wariant 2: Hardware Serial (USB / UART):**
  - Bezpośrednie wpięcie do modułu LoRa (np. Ebyte SX1262 przez UART) lub kabla USB.
- **Wariant 3: UDP / GSM (Opcjonalny):**
  - Przesyłanie ramek CRSF wewnątrz pakietu UDP.

### Krok 3: Zarządzanie Sprzętem i Fail-Safe (`PCA9685Manager.h`)
- Inicjalizacja magistrali I2C na 400 kHz (Fast Mode).
- `setTimeOut(10)` i funkcja autoodzyskiwania szyny (`checkAndRecoverI2C()`).
- Sterownik PCA9685:
  - Przeliczanie wartości 11-bit na ticks PCA (50–330 Hz).
  - Autokalibracja częstotliwości oscylatora PCA.
- **Twardy Fail-Safe:**
  - Timer braku ramki (>100 ms) lub sygnał DISARM (CH5 LOW):
    - Gaz (CH2) wymuszony na neutral `1500 µs` (lub `1000 µs` dla aut z hamulcem jednokierunkowym).
    - Skręt (CH1) wymuszony na środek `1500 µs`.

### Krok 4: Integracja Sensorów (`SensorsManager.h`)
- Warstwa abstrakcji IMU (MPU6050, MPU9250, BNO085, BMX160) ze sprzętowym filtrem DLPF.
- Obsługa GPS przez sprzętowy port UART (TinyGPS++).
- Pomiar napięcia na dedykowanym pinie ADC1 (GPIO 33) z filtrem EMA (Exponential Moving Average).
- Opcjonalne wyjście MSP OSD dla gogli cyfrowych Walksnail.

---

## 🛡️ Bezpieczeństwo i Niezawodność (Safety-Critical Checklist)
- [x] Zero blokujących pętli `delay()` w krytycznych zadaniach.
- [x] Całkowite odseparowanie komunikacji radiowej (Core 0) od pętli generowania PWM (Core 1).
- [x] Niezależny Task Watchdog Timer (WDT) 2s restartujący układ w przypadku deadlocka.
- [x] Ochrona przed niekontrolowanym odjazdem modelu (Neutral 1500 µs po 100 ms braku sygnału).
- [x] Używanie wyłącznie wejść z bloku ADC1 (brak kolizji z modułem Wi-Fi/ESP-NOW).

---

## 📝 Aktualizacja TODO.md
Po zakończeniu implementacji zaktualizować [TODO.md](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/TODO.md) o nowy moduł Tier 2 Pro (CRSF Multi-Link Hub).
