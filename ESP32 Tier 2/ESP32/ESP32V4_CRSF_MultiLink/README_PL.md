# RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub & Dual-Link Hybrid)

## 📌 Opis Modułu

Oprogramowanie układowe **Tier 2 Pro (V4)** dla mikrokontrolera **ESP32** to zaawansowany pokładowy hub sterowania, arbitrażu i telemetrii dla pojazdów RC / Roverów (np. ARRMA Mojave 4S). 

Łączy w sobie pełne wsparcie dla modelarskiego standardu **CRSF (Crossfire / ExpressLRS)** z łącznością długodystansową przez **Internet 5G / VPN (Tailscale)**.

### Główne Funkcjonalności:
1. **Tryb Dual-Link Hybrid (Inteligentny Muxer):**
   - **Tor 1: Bezpośredni link radiowy RF (Ultra Low-Latency):** Lokalna aparatura (np. RadioMaster MT12) sparowana z odbiornikiem **RadioMaster ER5C V2** podłączonym po sprzętowym **UART (420 000 baud)**.
   - **Tor 2: Łączność długodystansowa przez Internet (5G/VPN):** ESP32 łączy się z mobilnym hotspotem Wi-Fi w telefonie pokładowym (np. Redmi 15 5G), a pakiety sterujące ze stacji PC (RCSIM GCS) trafiają przez tunel Tailscale VPN / UDP.
   - **Automatyczny Arbiter:** Domyślnie priorytet ma lokalne radio RF. W przypadku utraty sygnału radiowego (>150 ms) lub wyłączenia aparatury, sterowanie płynnie przejmuje stacja PC przez 5G.
   - **Przełącznik trybów z aparatury (AUX2 / Kanał 6):**
     - Pozycja góra (< 1300 µs): Wymuszenie lokalnego radia RF.
     - Pozycja środek (1300–1700 µs): Tryb automatyczny (AUTO Muxer).
     - Pozycja dół (> 1700 µs): Wymuszenie sterowania przez Internet 5G.
2. **Dwukierunkowa Telemetria CRSF:**
   - ESP32 odczytuje czujniki pokładowe i generuje standardowe pakiety telemetrii CRSF (`0x1E Attitude`, `0x08 Battery`, `0x02 GPS`, `0x14 Link Statistics`).
   - Pakiety wysyłane są **symultanicznie**:
     - Do odbiornika ER5C V2 (dzięki czemu napięcie baterii, GPS i sztuczny horyzont wyświetlają się bezpośrednio na ekranie aparatury RadioMaster MT12).
     - Do stacji RCSIM na PC przez sieć 5G / VPN (dla wirtualnego kokpitu HUD i Force Feedback).
3. **Sterownik PCA9685 (I2C Fast Mode 400 kHz):**
   - Bezpośrednia obsługa serwa skrętu i regulatora ESC Spektrum Firma / Hobbywing.
   - Algorytm autoodzyskiwania szyny I2C (Bus Recovery) chroniący przed zakłóceniami EMI.
4. **Sprzętowe Bezpieczeństwo i Fail-Safe:**
   - W przypadku utraty obu torów łączności (>150 ms) lub wyłączenia uzbrojenia (Kanał 5 / AUX1 < 1350 µs) serwo i gaz są natychmiast ustawiane w pozycję neutralną (`1500 µs`).
   - Nieblokująca obsługa Wi-Fi: lokalne radio RF działa natychmiast od 1. milisekundy po włączeniu zasilania, nawet jeśli telefon z hotspotem nie został jeszcze uruchomiony.
   - Architektura **FreeRTOS Dual-Core**: Core 0 zajmuje się komunikacją i arbitrażem, Core 1 generowaniem PWM i czujnikami.

---

## 🔌 Schemat Połączeń (Pinout)

| Peryferium / Moduł | Pin ESP32 | Pin Modułu | Opis / Uwagi |
|---|---|---|---|
| **PCA9685 & IMU SDA** | `GPIO 13` | SDA | Szyna danych I2C (z rezystorami pull-up 4.7k) |
| **PCA9685 & IMU SCL** | `GPIO 14` | SCL | Szyna zegara I2C (Fast Mode 400 kHz) |
| **PCA Kalibracja** | `GPIO 12` | CH15 | Opcjonalne sprzężenie zwrotne do autokalibracji |
| **Odbiornik ER5C V2 TX** | `GPIO 16` (RX2) | CRSF TX | Odbiór ramek sterujących CRSF (420 000 bps) |
| **Odbiornik ER5C V2 RX** | `GPIO 17` (TX2) | CRSF RX | Wysyłanie telemetrii do aparatury MT12 |
| **GPS TX (NMEA)** | `GPIO 32` (RX1) | TXD | Odbiór danych GPS (9600 bps, NMEA) |
| **Bateria (VBAT)** | `GPIO 33` (ADC1) | Dzielnik | Dzielnik napięcia R1=10k, R2=2.2k (ratio 5.545) |
| **Zasilanie ESP32** | `5V / VIN` | BEC 5V | Zewnętrzny BEC 5V/2-3A (wspólna masa GND z ESC i serwem!) |

> [!IMPORTANT]
> **Masa (GND)** wszystkich komponentów (ESP32, odbiornik ER5C V2, PCA9685, regulator ESC, czujnik IMU, GPS) **musi być połączona wspólnie**.

---

## ⚙️ Konfiguracja Odbiornika RadioMaster ER5C V2 (ExpressLRS)

Aby odbiornik ER5C V2 przekazywał ramki CRSF do ESP32:
1. Połącz się z panelem Web UI odbiornika (przez Wi-Fi ExpressLRS lub aplikację ExpressLRS Configurator).
2. W zakładce **Model / Hardware**:
   - Ustaw **Pin 1 (lub dedykowany pin)** jako `CRSF TX` $\rightarrow$ połącz z `GPIO 16` (RX2) w ESP32.
   - Ustaw **Pin 2 (lub dedykowany pin)** jako `CRSF RX` $\rightarrow$ połącz z `GPIO 17` (TX2) w ESP32.
   - Baud rate: `420000` (domyślny CRSF).
3. Po włączeniu zasilania aparatura MT12 natychmiast wykryje sensory telemetryczne (w menu aparatury: *Telemetry -> Discover new sensors* znajdziesz `RxBt`, `GPS`, `Pitch`, `Roll`, `Yaw`, `RQly`).

---

## 📱 Konfiguracja Telefonu (Redmi 15 5G) i Sieci

1. **Hotspot Wi-Fi:**
   - Włącz hotspot osobisty w telefonie.
   - Ustaw SSID i Hasło zgodnie ze stałymi w pliku `ESP32V4_CRSF_MultiLink.ino`:
     ```cpp
     const char* WIFI_SSID     = "Redmi_Hotspot";
     const char* WIFI_PASSWORD = "twoje_haslo";
     ```
2. **Kamera i Wideo FPV:**
   - Telefon umieszczony w kokpicie auta transmituje strumień wideo o niskim opóźnieniu (WebRTC) bezpośrednio do stacji RCSIM na PC.
3. **Tailscale VPN (Dostęp przez Internet):**
   - Na telefonie i na PC domowym zainstaluj aplikację **Tailscale**.
   - ESP32 po połączeniu z hotspotem komunikuje się w wirtualnej sieci Tailnet (bez potrzeby publicznego adresu IP na karcie SIM).

---

## 🎮 Przypisanie Kanałów

| Kanał | Nazwa | Funkcja | Zakres wartości |
|---|---|---|---|
| **CH 1** | Steering | Skręt kół (Serwo na CH0 PCA9685) | 1000 µs (lewo) – 1500 µs – 2000 µs (prawo) |
| **CH 2** | Throttle | Gaz / Hamulec (ESC na CH1 PCA9685) | 1000 µs (wstecz) – 1500 µs (neutral) – 2000 µs (pełny gaz) |
| **CH 5** | AUX 1 | Uzbrojenie (ARM Switch) | > 1350 µs = ARMED, < 1350 µs = DISARMED (Stop) |
| **CH 6** | AUX 2 | Przełącznik Muxera (Radio vs 5G) | < 1300 µs = Wymuś RF, 1300-1700 = AUTO, > 1700 µs = Wymuś 5G |

---

## 🛠️ Wymagane Biblioteki

W Arduino IDE Library Manager:
- **Adafruit PWM Servo Driver Library**
- **Adafruit BusIO**
- **TinyGPSPlus**
- **MPU6050_light** (jeśli używany MPU6050)
