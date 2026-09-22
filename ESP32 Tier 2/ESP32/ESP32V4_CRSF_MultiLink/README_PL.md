# RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub)

## 📌 Opis Modułu

Oprogramowanie układowe **Tier 2 Pro (V4)** dla mikrokontrolera **ESP32** to zaawansowany pokładowy hub sterowania i telemetrii dla pojazdów RC/Roverów, integrujący:
- **Natywny protokół CRSF (Crossfire / ELRS)** ze sprzętową weryfikacją sumy kontrolnej **CRC8 DVB-S2**.
- **Wielowarstwową komunikację radiową**:
  - **ESP-NOW** (opóźnienia rzędu 1–2 ms, brak konieczności routera Wi-Fi, bezpośrednie parowanie MAC).
  - **Hardware Serial** (USB CDC na PC lub mostek UART do zewnętrznych modułów LoRa SX1262/SX1280).
  - **UDP** (tradycyjne Wi-Fi lub lokalny mostek GSM/LTE).
  - **MicroLink VPN (Tailscale / WireGuard):** Pełna integracja z biblioteką [CamM2325/microlink](https://github.com/CamM2325/microlink) – bezpieczne sterowanie i telemetria przez Internet / 4G LTE bez publicznego IP i bez przekierowywania portów!
- **Sterownik PCA9685**: Odświeżanie serw i regulatorów ESC na szynie I2C w trybie **Fast Mode (400 kHz)** z autoodzyskiwaniem w przypadku zakłóceń EMI.
- **Wieloczujnikową telemetrię CRSF**:
  - `0x1E Attitude`: Przechyły Pitch/Roll z akcelerometru/żyroskopu IMU (MPU6050/MPU9250 z filtrem DLPF).
  - `0x02 GPS`: Szerokość, długość, prędkość w km/h, kurs i liczba satelitów.
  - `0x08 Battery`: Napięcie akumulatora z filtrem wykładniczym EMA na bezpiecznym pinie ADC1.
  - `0x14 Link Statistics`: RSSI i jakość sygnału LQI.
- **Sprzętowe bezpieczeństwo (Fail-Safe & Dual-Core FreeRTOS)**:
  - Odseparowanie transmisji radiowej (**Core 0**) od pętli generowania PWM (**Core 1**).
  - Automatyczny powrót do pozycji neutralnej (`1500 µs`) po przekroczeniu 150 ms bez pakietu lub po wyłączeniu przełącznika ARM (Kanał 5 / AUX1).

---

## 🔌 Schemat Połączeń (Pinout)

| Peryferium / Moduł | Pin ESP32 | Opis / Uwagi |
|---|---|---|
| **I2C SDA** | `GPIO 13` | Szyna danych I2C (PCA9685, IMU MPU6050/9250) |
| **I2C SCL** | `GPIO 14` | Szyna zegara I2C (400 kHz) |
| **PCA Kalibracja** | `GPIO 12` | Opcjonalny sygnał pomiarowy z CH15 PCA9685 |
| **GPS TX -> ESP32 RX** | `GPIO 32` (RX1) | Odbiór NMEA (HardwareSerial 1, 9600-115200 bps) |
| **Bateria (VBAT)** | `GPIO 33` (ADC1) | Dzielnik napięcia R1=10k, R2=2.2k (stosunek ~5.545) |
| **Zasilanie ESP32** | `5V / VIN` | Zewnętrzny BEC 5V/2A (nie zasilać z portu 3.3V) |

---

## ⚙️ Konfiguracja i Wybór Medium Radiowego

W pliku `ESP32V4_CRSF_MultiLink.ino` w sekcji nagłówkowej wybierasz tryb pracy:

```cpp
// Opcje: TRANSPORT_ESP_NOW, TRANSPORT_SERIAL, TRANSPORT_UDP, TRANSPORT_MICROLINK_VPN
#define ACTIVE_TRANSPORT     TRANSPORT_ESP_NOW

// Jeśli wybrano TRANSPORT_MICROLINK_VPN:
#define TAILSCALE_AUTH_KEY   "tskey-auth-YOUR_AUTH_KEY_HERE"
#define GCS_TAILSCALE_IP     IPAddress(100, 64, 0, 1) // IP stacji GCS w sieci Tailnet
```

1. **ESP-NOW:** Domyślny i rekomendowany tryb do jazdy na torze. Daje najniższe możliwe opóźnienia (**1–2 ms**).
2. **Serial:** Przeznaczony do połączenia kablem USB z PC lub modułów LoRa UART.
3. **UDP:** Przeznaczony do jazdy w lokalnej sieci Wi-Fi.
4. **MicroLink VPN:** Globalne sterowanie pojazdem przez sieć **Tailscale (WireGuard)** z dowolnego miejsca na świecie przez modem 4G/LTE lub mobilny hotspot!

---

## 🛠️ Wymagane Biblioteki (Arduino IDE)

W menedżerze bibliotek zainstaluj:
- **Adafruit PWM Servo Driver Library** (Adafruit)
- **Adafruit BusIO** (Adafruit)
- **TinyGPSPlus** (Mikal Hart)
- **MPU6050_light** (rfetick)

---

## 🚀 Ustawienia Kompilatora

- **Płytka:** `ESP32 Dev Module` lub `ESP32 Wrover Module`
- **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)`
- **Upload Speed:** `115200`
