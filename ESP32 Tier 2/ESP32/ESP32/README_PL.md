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

## 🛠️ Wymagane Biblioteki

Do poprawnej kompilacji w Arduino IDE wymagane są biblioteki:

- `Adafruit PWMServoDriver Library`
- `MPU6050_light`
- `esp_camera` (wbudowana w zestaw ESP32 Board Manager)

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
