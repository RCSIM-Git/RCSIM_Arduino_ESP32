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

## 📜 Kod Źródłowy, Patch i Licencja GNU GPLv3

- **Projekt bazowy:** [ExpressLRS/ExpressLRS](https://github.com/ExpressLRS/ExpressLRS) (gałąź `master`, commit bazowy `5909f77`).
- **Zgodność z GPLv3 (Kod źródłowy):** Zgodnie z sekcją 6 licencji GNU General Public License v3.0, pełny zestaw modyfikacji w kodzie źródłowym C++ i JavaScript (obsługa IMU MPU9250/MPU6050, Auto-Baudrate GPS, ochrona pinu PWM dla regulatora ESC na CH2 oraz rozpięcie parowania UART w WebUI) udostępniony jest w postaci pliku patch:
  [`elrs_v4.1_er5cv2_gps_imu.patch`](./elrs_v4.1_er5cv2_gps_imu.patch)
- **Aplikacja patcha na czyste repozytorium ExpressLRS:**
  ```bash
  git clone https://github.com/ExpressLRS/ExpressLRS.git
  cd ExpressLRS
  git checkout 5909f77
  git apply elrs_v4.1_er5cv2_gps_imu.patch
  ```
- **Licencja:** GNU General Public License v3.0 (GPLv3).
- **Zastrzeżenie (Disclaimer):** Niniejsze oprogramowanie stanowi niezależną, społecznościową modyfikację opracowaną na potrzeby projektu RCSIM. Oprogramowanie jest rozpowszechniane bez jakiejkolwiek gwarancji (AS IS). Autorzy nie ponoszą odpowiedzialności za jakiekolwiek szkody powstałe w wyniku jego użytkowania w modelach RC. Znaki towarowe ExpressLRS oraz RadioMaster należą do ich prawnych właścicieli.
