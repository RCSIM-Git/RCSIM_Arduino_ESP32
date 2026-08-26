# RCSIM - ESP32 Hub Sterowania V3.1 (WiFi Lite & Camera Edition)

## 📌 Opis Projektu

Szkic `ESP32.ino` przekształca płytkę **ESP32 / ESP32-CAM** w lekki, stabilny hub sterująco-telemetryczny dla stacji naziemnej **RCSIM (Radio Control Simulators)**. Moduł odbiera polecenia sterujące po protokole UDP z komputera GCS, steruje 16 serwomechanizmami lub regulatorami ESC poprzez magistralę I2C i układ **PCA9685**, strumieniuje wideo w formacie MJPEG z wbudowanej kamery oraz przesyła podstawową telemetrię z czujnika IMU (**MPU6050**).

Wersja 3.1 obsługuje zarówno statyczną konfigurację IP, jak i dynamiczne pobieranie adresu z DHCP, oraz posiada elastyczne definicje pinów dla najpopularniejszych płyt deweloperskich (AI-Thinker, Wrover-Dev, LilyGO T-SIMCAM S3).

---

## 🚀 Kluczowe Funkcjonalności

1. **Sterowanie PWM (PCA9685 - 16 Kanałów)**:
   - Odbiór 16-kanalowych pakietów UDP (port `12345`) i konwersja wartości sygnału (1000 - 2000 us) na wartości rejestrów PWM (205 - 410 ticks przy 50 Hz).
2. **Strumieniowanie Wideo MJPEG (Dual-Core FreeRTOS)**:
   - Dedykowany serwer HTTP na porcie `81` (`http://<IP>:81/stream`).
   - Wątek wideo (`videoTask`) uruchamiany jest na **Core 0**, co chroni pętlę sterowania serwami (Core 1) przed spowolnieniami.
3. **Telemetria IMU (MPU6050)**:
   - Wysyłanie pakietów JSON z przyspieszeniami (`ax`, `ay`, `az`) oraz prędkościami kątowymi (`gx`, `gy`, `gz`) na port `12347` stacji naziemnej z częstotliwością 20 Hz (co 50 ms).
4. **Failsafe & Bezpieczeństwo**:
   - W przypadku braku ramki sterującej przez ponad 500 ms (`FAILSAFE_TIMEOUT_MS`), system automatycznie ustawia wszystkie kanały w pozycji neutralnej (1500 us).
5. **Wsparcie dla Profili Sprzętowych**:
   - Wbudowana konfiguracja pinów dla płyt `BOARD_AI_THINKER`, `BOARD_WROVER_DEV` oraz `BOARD_LILYGO_TSIMCAM_S3`.

---

## 🔌 Schemat Połączeń i Pinout

### Szyna I2C (PCA9685 & MPU6050)

| Płytka / Profil | Pin SDA | Pin SCL | Uwagi |
|---|---|---|---|
| **BOARD_WROVER_DEV** | `GPIO 13` | `GPIO 14` | Przeniesione z 21/22, aby uniknąć kolizji z SCCB kamery |
| **BOARD_AI_THINKER** | `GPIO 21` | `GPIO 22` | Standardowe przypisanie dla AI-Thinker |
| **LILYGO_TSIMCAM_S3** | `GPIO 21` | `GPIO 46` | Dedykowana obsługa szyny S3 |

---

## 📡 Format Telemetrii UDP (JSON)

Pakiet przesyłany z ESP32 na adres IP komputera (port **12347**):

```json
{
  "imu": {
    "ax": 0.02,
    "ay": -0.01,
    "az": 1.00,
    "gx": 0.10,
    "gy": -0.05,
    "gz": 0.00
  }
}
```

---

## 🛠️ Instalacja i Konfiguracja Środowiska (Arduino IDE)

### 1. Dodanie obsługi płytek ESP32
1. Otwórz Arduino IDE i wejdź w **Plik** -> **Preferencje** (`Ctrl + ,`).
2. W polu **Dodatkowe adresy URL do menedżera płytek** (Additional Boards Manager URLs) wklej oficjalny adres:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Otwórz **Menedżer Płytek** (**Narzędzia** -> **Płytka** -> **Menedżer płytek...** lub ikona płytki na bocznym pasku).
4. Wyszukaj frazę `esp32` (autor: *Espressif Systems*) i kliknij **Zainstaluj** (zalecana wersja 2.0.x / 3.x).

---

### 2. Wymagane Biblioteki (Menedżer Bibliotek)
Otwórz **Menedżer Bibliotek** (**Narzędzia** -> **Zarządzaj bibliotekami...** / `Ctrl + Shift + I`) i zainstaluj:

- **Adafruit PWM Servo Driver Library** (autor: *Adafruit*) — do sterowania modułem PCA9685 po I2C.
- **Adafruit BusIO** (autor: *Adafruit*) — wymagana zależność dla bibliotek Adafruit.
- **MPU6050_light** (autor: *rfetick*) — lekka i szybka obsługa czujnika żyroskopu/akcelerometru MPU6050.
- *Uwaga:* Biblioteka `esp_camera` oraz `WiFi` / `WiFiUdp` / `Wire` są wbudowane bezpośrednio w pakiet płytki ESP32 (Board Support Package) i nie wymagają instalacji z zewnątrz.
- *Opcjonalnie (dla innych wariantów / funkcji):* `AsyncTCP`, `ESPAsyncWebServer`, `PPMEncoder`.

---

### 3. Ustawienia Kompilatora i Wgrywania (Menu Narzędzia / Tools)
Aby kompilacja i wgrywanie przebiegły bez błędów, ustaw parametry w menu **Narzędzia** według poniższej konfiguracji:

| Parametr w menu Narzędzia | Wartość | Opis / Dlaczego tak |
|---|---|---|
| **Board (Płytka)** | `"ESP32 Wrover Module"` lub `"AI Thinker ESP32-CAM"` | Zależnie od posiadanej płytki (dla Freenove/WROVER wybierz Wrover Module) |
| **Partition Scheme** | **`"Huge APP (3MB No OTA/1MB SPIFFS)"`** | **KRYTYCZNE!** Domyślna partycja nie pomieści kodu ze stosem Wi-Fi, MJPEG i kamerą |
| **Flash Frequency** | `40MHz` | Zapewnia stabilną pracę pamięci Flash SPI |
| **Flash Mode** | `DIO` | Standardowy, bezpieczny tryb odczytu pamięci Flash |
| **Core Debug Level** | `None` | Wyłącza zbędny narzut diagnostyczny na porcie szeregowym |
| **Erase All Flash Before Sketch Upload** | `Disabled` | Standardowe wgrywanie |
| **Upload Speed** | `115200` (lub `921600`) | `115200` zapobiega błędom CRC/timeout przy tanich konwerterach UART |
| **Port** | Wybierz właściwy port (np. `COM10`) | Port szeregowy podłączonego konwertera lub płytki ESP32 |

---

### 4. Procedura Wgrywania Firmware (ESP32-CAM Gotchas)
W przypadku popularnych płytek **AI-Thinker ESP32-CAM** (bez wbudowanego portu USB-UART):
1. **Wejście w tryb Bootloadera:** Połącz zworką pin **`GPIO 0` (IO0)** z pinem **`GND`**.
2. Wciśnij przycisk **`RST` (Reset)** na płytce ESP32-CAM (lub odłącz i podłącz zasilanie).
3. W Arduino IDE kliknij **Wgraj** (Upload / `Ctrl + U`).
4. **Uruchomienie programu:** Po zakończeniu wgrywania ("Done uploading"), **odłącz pin `GPIO 0` od `GND`**, a następnie ponownie wciśnij przycisk **`RST`**, aby uruchomić wgrany firmware.
5. **Zasilanie:** Płytki ESP32 z włączoną kamerą i Wi-Fi pobierają w impulsach do 500mA. Zapewnij stabilne zasilanie 5V (np. z zewnętrznego BEC-a 5V/2A), unikając zasilania bezpośrednio ze słabych linii 3.3V konwertera USB.

---

## ⚙️ Konfiguracja Sprzętowa i Flagi

Główne przełączniki w nagłówku pliku `ESP32.ino`:

```cpp
#define ENABLE_CAMERA true    // Włącz strumień wideo MJPEG (port 81)
#define ENABLE_IMU    true    // Włącz wysyłanie danych z czujnika IMU (port 12347)

// Wybór aktywnego profilu płytki:
//#define BOARD_AI_THINKER
#define BOARD_WROVER_DEV
//#define LILYGO_TSIMCAM_S3
```
