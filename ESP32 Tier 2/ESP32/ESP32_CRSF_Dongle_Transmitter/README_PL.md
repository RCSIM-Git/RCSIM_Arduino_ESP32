# RCSIM - Nadajnik USB ESP-NOW CRSF (PC USB Dongle)

## 📌 Opis i Rola w Systemie

Niniejszy program jest przeznaczony dla **drugiego mikrokontrolera ESP32**, który pełni rolę **bezprzewodowego adaptera radiowego USB (Dongle)** wpiętego bezpośrednio do komputera PC ze stacją naziemną **RCSIM GCS**.

### Zasada Działania (Pełny Dupleks):
1. **Tor Sterowania (PC -> Model):**
   - GCS generuje ramki protokołu **CRSF** (`0x16 RC_CHANNELS_PACKED` z 16 kanałami 11-bit) i wysyła je przez wirtualny port COM (USB Serial, 115200 bps).
   - ESP32 odbiera ramkę i natychmiast, w ciągu mikrosekund, transmituje ją drogą radiową przez **ESP-NOW** bezpośrednio do odbiornika w modelu RC.
2. **Tor Telemetrii (Model -> PC):**
   - Model odsyła ramki telemetrii CRSF (`0x1E Attitude`, `0x02 GPS`, `0x08 Battery`, `0x14 Link Stats`) przez ESP-NOW.
   - Dongle odbiera pakiet i bezzwłocznie wypycha go na port USB do aplikacji GCS.
   - GCS automatycznie przetwarza dane: telemetria IMU zasila **Force Feedback (kierownice Sim-Racing)**, a współrzędne GPS nanoszone są na mapę.

---

## 🚀 Zalety Tego Rozwiązania
- **Skrajnie niskie opóźnienie (Ultra-Low Latency):** Czas przelotu pakietu w warstwie MAC wynosi **1–2 ms**.
- **Całkowita niezależność od routera Wi-Fi:** Brak logowania do sieci domowej, brak konfliktów IP, brak utraty pakietów przy obciążeniu domowej sieci przez innych domowników.
- **Standaryzacja w GCS:** Aplikacja PC nie musi wiedzieć, że rozmawia przez ESP-NOW – z perspektywy GCS jest to zwykłe łącze CRSF (takie samo jak profesjonalny moduł ExpressLRS / Nomad).

---

## 🛠️ Wgrywanie i Uruchomienie

1. Podłącz drugie ESP32 (dowolny model: ESP32-WROOM-32, NodeMCU, ESP32-S3 itp.) do wolnego portu USB w komputerze.
2. W Arduino IDE otwórz plik `ESP32_CRSF_Dongle_Transmitter.ino`.
3. Wybierz płytkę **`ESP32 Dev Module`** oraz właściwy port COM.
4. Kliknij **Wgraj** (`Ctrl + U`).
5. Po wgraniu w aplikacji **RCSIM GCS**:
   - W ustawieniach połączenia wybierz protokół **CRSF Direct** (lub Tier 3 Profile).
   - Wybierz numer portu COM Twojego Dongla (np. `COM7`).
   - Prędkość: `115200 bps`.
   - Kliknij **Połącz** i **Uzbrój (ARM)** pojazd!
