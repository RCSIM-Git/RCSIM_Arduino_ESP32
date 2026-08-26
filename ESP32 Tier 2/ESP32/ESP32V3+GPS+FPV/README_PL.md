# RCSIM - ESP32 Hub Sterowania V3.3 (Full OSD & GPS Edition)

## 📌 Opis Projektu

Niniejszy moduł oprogramowania układowego (Firmware) dla mikrokontrolera **ESP32** pełni rolę bezprzewodowego huba sterującego i telemetrycznego dla systemu **RCSIM (Radio Control Simulators)**. Układ odpowiada za odbiór sygnałów sterujących po UDP, generowanie sygnałów PWM dla serwomechanizmów (za pośrednictwem kontrolera PCA9685), przesyłanie strumienia wideo MJPEG z kamery FPV, odczyt i parsowanie danych z czujnika GPS oraz transmisję telemetrii w czasie rzeczywistym zarówno do stacji naziemnej GCS (PC), jak i do gogli FPV (Walksnail MSP OSD).

Moduł zaprojektowano zgodnie ze standardami bezpieczeństwa fizycznego i niezawodności (*Safety-Critical*), eliminując ryzyko zawieszenia pętli sterującej w trudnych warunkach zakłóceń elektromagnetycznych (EMI).

---

## 🚀 Kluczowe Funkcjonalności

1. **Uniwersalna Warstwa Abstrakcji IMU (IMU Abstraction Layer)**:
   - Wsparcie dla czujników: **MPU6050**, **MPU9250 / GY-91**, **BNO08x** (BNO080/085 - sprzętowa fuzja kwaternionów), **BMX160**.
   - Przełączanie aktywnego czujnika za pomocą jednej stałej `#define ACTIVE_IMU`.
   - Zintegrowany sprzętowy filtr cyfrowy DLPF (~42Hz dla MPU6050/MPU9250) zapobiegający zakłóceniom od drgań silnika.

2. **Automatyczne Odzyskiwanie Magistrali I2C & Limit Czasu**:
   - Limit czasu oczekiwania I2C ustawiony na 10 ms (`Wire.setTimeOut(10)`).
   - Mechanizm `checkAndRecoverI2C()` automatycznie wykrywa błędy magistrali i przeprowadza jej szybki restart w czasie rzeczywistym bez resetu całego ESP32.

3. **Sprzętowy Psu Stróżujący (Task Watchdog Timer - WDT)**:
   - Zaimplementowany dwusekundowy timer WDT monitorujący rdzeń pętli `loop()` (Core 1).
   - Zabezpieczenie przed trwałym zamrożeniem systemu i utratą kontroli nad pojazdem (*Flyaway / Runaway Protection*).

4. **Natywna Integracja Walksnail MSP OSD**:
   - Wysyłanie ramek protokołu MSP (MultiWii Serial Protocol) bezpośrednio do nadajnika VTX:
     - `MSP_ATTITUDE` (Cmd 108, 20 Hz) – horyzont i kąty nachylenia.
     - `MSP_ANALOG` (Cmd 110, 5 Hz) – napięcie akumulatora głównego i RSSI Wi-Fi.
     - `MSP_STATUS` (Cmd 101, 5 Hz) – status uzbrojenia (ARM/DISARM na podstawie Failsafe).
     - `MSP_RAW_GPS` (Cmd 106, 5 Hz) – pozycja GPS, wysokość, prędkość i liczba satelitów.
     - `MSP_NAME` (Cmd 10, 0.5 Hz) – identyfikator pojazdu ("RCSIM V3.3").

5. **Strumieniowanie Wideo MJPEG (Dual Core FreeRTOS)**:
   - Dedykowany wątek `videoTask` przypisany do **Core 0** (port HTTP 81: `/stream`).
   - Całkowite odseparowanie obciążenia graficznego od krytycznej pętli sterowania sterami na Core 1.

6. **Bezblokujący Odczyt Napięcia Akumulatora (IIR / EMA Filter)**:
   - Zastosowanie dolnoprzepustowego wykładniczego filtra ruchomego (Exponential Moving Average) na pinie ADC1 (GPIO 33).
   - Zerowy narzut czasowy (redukcja czasu próbkowania z 250 us do 10-15 us), brak mikro-jitteru w pętli PWM.

7. **Autokalibracja Pętli Sprzężenia Zwrotnego PCA9685**:
   - Pętla pomiarowa (`calibratePCA9685`) wykorzystująca sygnał zwrotny z kanału 15 PCA9685 do pinu GPIO 12 ESP32 (`pulseIn`).
   - Dynamiczne dostrajanie wewnętrznej częstotliwości oscylatora z tolerancją do 3 us.

8. **Failsafe & Zabezpieczenia Sterowania**:
   - Wymuszenie neutralnego sygnału PWM (1500 us) na wszystkich 16 kanałach w przypadku braku pakietu sterującego UDP przez ponad 500 ms.

---

## 🔌 Schemat Połączeń (Pinout - WROVER-DEV)

| Peryferium / Moduł | Pin ESP32 | Opis / Uwagi |
|---|---|---|
| **I2C SDA** | `GPIO 13` | Szyna danych I2C (PCA9685, IMU) |
| **I2C SCL** | `GPIO 14` | Szyna zegarowa I2C |
| **Walksnail VTX TX** | `GPIO 2` (TX2) | Połączenie z Walksnail RX |
| **Walksnail VTX RX** | `GPIO 15` (RX2) | Połączenie z Walksnail TX |
| **GPS TX** | `GPIO 32` (RX1) | Odbiór NMEA (HardwareSerial 1) |
| **Pomiar Napięcia VBAT**| `GPIO 33` (ADC1) | Dzielnik napięcia R1=10k, R2=2.2k (~5.545x) |
| **PCA Kalibracja IN** | `GPIO 12` | Sygnał pomiarowy z kanału CH15 PCA9685 |
| **Kamera XCLK** | `GPIO 21` | Zegar magistrali kamery |
| **Kamera SIOD / SIOC** | `GPIO 26 / 27` | SCCB / I2C kamery |

---

## 📡 Format Telemetrii UDP (JSON)

ESP32 przesyła pakiety JSON na adres IP komputera stacji naziemnej na port **12347** z częstotliwością 20 Hz:

```json
{
  "imu": {
    "ax": 0.02,
    "ay": -0.01,
    "az": 1.00,
    "gx": 0.10,
    "gy": -0.05,
    "gz": 0.00,
    "roll": 1.2,
    "pitch": -0.5,
    "yaw": 180.0
  },
  "gps": {
    "valid": true,
    "lat": 52.229675,
    "lng": 21.012230,
    "alt": 120.5,
    "speed": 15.4,
    "sats": 10
  },
  "vbat": 12.42,
  "rssi": -55
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
3. Otwórz **Menedżer Płytek** (**Narzędzia** -> **Płytka** -> **Menedżer płytek...**).
4. Wyszukaj `esp32` (autor: *Espressif Systems*) i zainstaluj najnowszą stabilną wersję (2.0.x / 3.x).

---

### 2. Wymagane Biblioteki (Menedżer Bibliotek)
Otwórz **Menedżer Bibliotek** (**Narzędzia** -> **Zarządzaj bibliotekami...** / `Ctrl + Shift + I`) i zainstaluj:

- **Adafruit PWM Servo Driver Library** (autor: *Adafruit*) — sterownik I2C układu PCA9685.
- **Adafruit BusIO** (autor: *Adafruit*) — wymagana zależność magistrali dla bibliotek Adafruit.
- **TinyGPSPlus** (autor: *Mikal Hart*) — obsługa i parsowanie ramek NMEA z modułu GPS.
- **Zależnie od wybranego czujnika IMU (`ACTIVE_IMU`)**:
  - Dla `IMU_TYPE_MPU6050` (domyślny): **MPU6050_light** (autor: *rfetick*).
  - Dla `IMU_TYPE_MPU9250`: **MPU9250_WE** (autor: *Wolfgang Ewald*).
  - Dla `IMU_TYPE_BNO08X`: **Adafruit BNO08x** (autor: *Adafruit*).
  - Dla `IMU_TYPE_BMX160`: **DFRobot_BMX160** (autor: *DFRobot*).
- *Uwaga:* Biblioteki `esp_camera`, `esp_task_wdt`, `WiFi`, `WiFiUdp`, `Wire` są integralną częścią rdzenia ESP32.
- *Opcjonalnie (dla innych rozszerzeń):* `AsyncTCP`, `ESPAsyncWebServer`, `PPMEncoder`.

---

### 3. Ustawienia Kompilatora i Wgrywania (Menu Narzędzia / Tools)

| Parametr w menu Narzędzia | Wartość | Opis / Dlaczego tak |
|---|---|---|
| **Board (Płytka)** | `"ESP32 Wrover Module"` lub `"AI Thinker ESP32-CAM"` | Wybierz Wrover Module dla płytek z PSRAM (Freenove / WROVER) |
| **Partition Scheme** | **`"Huge APP (3MB No OTA/1MB SPIFFS)"`** | **KRYTYCZNE!** Wymagane ze względu na rozmiar stosu Wi-Fi, GPS, OSD i kamery |
| **Flash Frequency** | `40MHz` | Stabilna praca pamięci Flash |
| **Flash Mode** | `DIO` | Bezpieczny tryb dostępu do Flash |
| **Core Debug Level** | `None` | Brak zbędnego narzutu na portach UART |
| **Erase All Flash Before Sketch Upload** | `Disabled` | Standardowe programowanie pamięci |
| **Upload Speed** | `115200` (lub `921600`) | `115200` zapobiega błędom transmisji przy programatorach USB-UART |
| **Port** | Wybierz właściwy port COM (np. `COM10`) | Port szeregowy podłączonej płytki / programatora |

---

### 4. Procedura Wgrywania Firmware (ESP32-CAM Gotchas)
1. **Wejście w tryb Bootloadera:** Połącz pin **`GPIO 0` (IO0)** z masą **`GND`**.
2. Wciśnij przycisk **`RST`** na płytce ESP32.
3. W Arduino IDE kliknij **Wgraj** (Upload / `Ctrl + U`).
4. Po zakończeniu wgrywania: **odłącz `GPIO 0` od `GND`** i wciśnij **`RST`**, aby uruchomić układ.
5. **Zasilanie:** Cały układ z kamerą, GPS, OSD i Wi-Fi wymaga stabilnego zasilania 5V (np. zewnętrzny BEC 5V/2A). Unikaj zasilania bezpośrednio ze słabych linii 3.3V konwerterów USB-UART.

---

## ⚙️ Konfiguracja Sprzętowa i Flagi

Przełączniki główne w nagłówku pliku:

```cpp
#define ENABLE_CAMERA                true   // Włączenie strumienia MJPEG
#define ENABLE_IMU                   true   // Włączenie obsługi czujnika IMU
#define ACTIVE_IMU                   IMU_TYPE_MPU6050 // Wybór typu czujnika IMU
#define ENABLE_PCA_AUTOCALIBRATION   true   // Włączenie autokalibracji częstotliwości PCA
#define ENABLE_WALKSNAIL_OSD         true   // Włączenie ramek OSD dla Walksnail
#define ENABLE_BATTERY               true   // Włączenie odczytu ADC akumulatora
#define ENABLE_GPS                   true   // Włączenie odczytu NMEA z GPS
```
