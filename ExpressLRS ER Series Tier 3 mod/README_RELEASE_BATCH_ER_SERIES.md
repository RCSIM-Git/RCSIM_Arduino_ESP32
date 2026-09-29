# 🛰️ RadioMaster ER Series (ER4 / ER5 / ER6 / ER8) – Custom ELRS v4.1 Firmware with GPS + IMU Telemetry

Niniejszy pakiet zawiera zmodyfikowany firmware **ExpressLRS v4.1.0** z natywną obsługą **GPS (NMEA/UBLOX z auto-baudrate)** oraz **czujników IMU (MPU9250 / MPU6050)** przesyłających dane telemetryczne (ramki CRSF `0x02`, `0x03`, `0x86`) prosto do stacji bazowej **RCSIM GCS / MCS** oraz aparatury RC (**EdgeTX**).

---

## 📦 Zestawienie wygenerowanych plików binarnych (Katalog `release/`)

Wszystkie pliki zostały przygotowane jako oficjalne pakiety **Unified Configuration** (posiadają wstrzykniętą fabryczną mapę pinów, profile mocy nadawania RF oraz bezpieczne domyślne opcje, gotowe do wgrania przez stronę Wi-Fi WebUI):

| Model Odbiornika | MCU | Format WebUI OTA | Format bezpośredni (FTDI/esptool) | Główne cechy sprzętowe |
| :--- | :---: | :--- | :--- | :--- |
| **RadioMaster ER4** | ESP8285 | [`ELRS_V4.1_RadioMaster_ER4_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER4_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER4_GPS_IMU.bin` | 4x PWM, lekki odbiornik samolotowy/kołowy |
| **RadioMaster ER5A / ER5C (V1)** | ESP8285 | [`ELRS_V4.1_RadioMaster_ER5A_C_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER5A_C_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER5A_C_GPS_IMU.bin` | 5x PWM |
| **RadioMaster ER5A / ER5C (V2)** | ESP8285 | [`ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin` | 5x PWM, zoptymalizowany dzielnik Vbat |
| **RadioMaster ER6** | ESP32 | [`ELRS_V4.1_RadioMaster_ER6_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER6_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER6_GPS_IMU.bin` | 6x PWM, True Diversity, UART + I2C |
| **RadioMaster ER6-G** | ESP32 | [`ELRS_V4.1_RadioMaster_ER6G_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER6G_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER6G_GPS_IMU.bin` | 6x PWM, zintegrowany Vario / Baro |
| **RadioMaster ER6-GV** | ESP32 | [`ELRS_V4.1_RadioMaster_ER6GV_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER6GV_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER6GV_GPS_IMU.bin` | 6x PWM, Vario + wejście pomiaru Vbat |
| **RadioMaster ER8** | ESP32 | [`ELRS_V4.1_RadioMaster_ER8_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER8_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER8_GPS_IMU.bin` | 8x PWM, True Diversity |
| **RadioMaster ER8-G** | ESP32 | [`ELRS_V4.1_RadioMaster_ER8G_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER8G_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER8G_GPS_IMU.bin` | 8x PWM, zintegrowany Vario / Baro |
| **RadioMaster ER8-GV** | ESP32 | [`ELRS_V4.1_RadioMaster_ER8GV_GPS_IMU.bin.gz`](file:///c:/Users/Mateusz/Desktop/er5c/release/ELRS_V4.1_RadioMaster_ER8GV_GPS_IMU.bin.gz) | `ELRS_V4.1_RadioMaster_ER8GV_GPS_IMU.bin` | 8x PWM, Vario + wejście pomiaru Vbat |

---

## 🔌 Podłączenie GPS i IMU w modelach ER6 i ER8 (ESP32)

W modelach **ER6** oraz **ER8** (ESP32) podłączenie jest jeszcze wygodniejsze niż w ER5:
1. **Dedykowany port CRSF/UART (złącze 4-pin JST-GH lub piny szpilkowe):**
   - Odbiornik posiada fizycznie niezależne linie `RX` (GPIO3) i `TX` (GPIO1).
   - Wystarczy podłączyć przewód **TX z modułu GPS** do pinu **RX odbiornika**.
   - Pin TX odbiornika nie koliduje z kanałami PWM serw!
2. **Magistrala I2C dla IMU (MPU9250 / MPU6050):**
   - Fabrycznie zdefiniowane linie: `SDA` = **GPIO23**, `SCL` = **GPIO18** (w wersjach ze zintegrowanym Vario barometr współdzieli magistralę I2C, więc MPU9250 podłącza się równolegle pod ten sam bus!).
3. **Konfiguracja w WebUI (`http://10.0.0.1`):**
   - W zakładce **Hardware / Model Configuration** można przypisać rolę poszczególnych pinów wedle uznania użytkownika.

---

## 🚀 Instrukcja aktualizacji przez Wi-Fi (WebUI)

1. Włącz odbiornik i odczekaj ok. 60 sekund (bez włączania aparatury), aż dioda LED zacznie szybko migać (tryb Access Point Wi-Fi).
2. Połącz się z siecią Wi-Fi:
   - **SSID:** `ExpressLRS RX`
   - **Hasło:** `expresslrs`
3. Otwórz w przeglądarce stronę: `http://10.0.0.1`
4. Przejdź do zakładki **Update**, wybierz odpowiedni dla posiadanego modelu plik `.bin.gz` z katalogu `release/` i kliknij **Flash**.
5. Po restarcie odbiornik natychmiast rozpoczyna nasłuch NMEA/GPS z automatycznym wykrywaniem prędkości (9600 - 115200 bps) oraz wysyłanie telemetrycznych ramek CRSF.

---

## 📜 Kod Źródłowy i Licencja GNU GPLv3

- **Projekt bazowy:** [ExpressLRS/ExpressLRS](https://github.com/ExpressLRS/ExpressLRS) (commit bazowy `5909f77`).
- **Kompletny kod źródłowy (GPLv3 §6):** Pełne drzewo kodu źródłowego zawierające wszystkie modyfikacje C++ i WebUI (`src/lib/IMU/`, Auto-Baudrate GPS, ochrona ESC PWM na CH2/GPIO1, uncoupling w WebUI) dostępne jest w repozytorium:
  - Repozytorium: [RCSIM-Git/expresslrs-er-series-telemetry](https://github.com/RCSIM-Git/expresslrs-er-series-telemetry)
  - Gałąź: `feat/rm-er-series-gps-imu-telemetry`
  - Trwały commit: [`de876c0286c2788b4e02fac267078c374501f0ec`](https://github.com/RCSIM-Git/expresslrs-er-series-telemetry/commit/de876c0286c2788b4e02fac267078c374501f0ec)
  - Wszystkie opublikowane w tym pakiecie pliki binarne zostały skompilowane z tego pojedynczego stanu źródeł.
- **Instrukcja kompilacji:** Zobacz [`BUILD_INSTRUCTIONS.md`](./BUILD_INSTRUCTIONS.md) dla szczegółowej procedury budowy w PlatformIO.
- **Plik patch:** [`elrs_v4.1_er5cv2_gps_imu.patch`](./elrs_v4.1_er5cv2_gps_imu.patch) jest również dołączony pomocniczo.
- **Licencja:** GNU General Public License v3.0 (GPLv3), zgodnie z licencją projektu nadrzędnego ExpressLRS.
- **Zastrzeżenie (Disclaimer):** Niniejsze oprogramowanie stanowi niezależną, społecznościową modyfikację opracowaną na potrzeby projektu RCSIM. Oprogramowanie jest rozpowszechniane bez jakiejkolwiek gwarancji (AS IS). Autorzy nie ponoszą odpowiedzialności za jakiekolwiek szkody powstałe w wyniku jego użytkowania w modelach RC. Znaki towarowe ExpressLRS oraz RadioMaster należą do ich prawnych właścicieli.
