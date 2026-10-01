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

## 🛒 Lista Materiałów i Komponentów (BOM — Stan Fizyczny z Paczek)

Poniższa lista odzwierciedla **fizycznie dostarczone i zweryfikowane komponenty** dla projektu **ARRMA Mojave 4S / 5G** w ramach ekosystemu RCSIM:

### 1. Aparatura i Łączność RC:
| Lp. | Komponent | Model / Oznaczenie | Ilość | Rola / Zastosowanie |
|---|---|---|:---:|---|
| 01 | **Aparatura pistoletowa RC** | **RadioMaster MT12 ELRS 2.4 GHz** | 1 zest. | Nadajnik kierowcy (EdgeTX + wbudowany moduł ExpressLRS) |
| 02 | **Odbiorniki modelarskie** | **RadioMaster ER3C-i** oraz **ER5C-i** | po 1 szt. | Odbiór CRSF (420 000 bps) i zwrotna telemetria pokładowa |

### 2. Główny Sterownik i Elektronika Pokładowa (Ekosystem Grove / I2C):
| Lp. | Komponent | Model / Oznaczenie | Ilość | Rola / Zastosowanie |
|---|---|---|:---:|---|
| 03 | **Mikrokontroler główny** | **ESP32-S3 DevKit (N8R8 / N16R8 Waveshare)** | 1 szt. | Dual-Link Muxer, FreeRTOS Dual-Core, Wi-Fi 5G + UART |
| 04 | **Terminal Shield** | **Adapter 44-pin ze złączami śrubowymi ARK** | 1 szt. | Wyprowadzenie pinów bez lutowania, odporne na wstrząsy |
| 05 | **Multiplexer I2C** | **Grove Hub I2C TCA9548A (8 portów)** | 1 szt. | Separacja szyny I2C i bezproblemowe łączenie peryferiów |
| 06 | **Sterownik serw i ESC** | **Grove PCA9685 (16-kanałowy PWM 12-bit)** | 1 szt. | Precyzyjne sterowanie serwem skrętu i regulatorem ESC |
| 07 | **Czujnik IMU 6-DoF** | **Grove LSM6DS3 (akcelerometr + żyroskop)** | 1 szt. | Telemetria przechyłów (Pitch/Roll/Yaw) i przeciążeń |
| 08 | **Moduł nawigacji GPS** | **Grove GPS Air530 z anteną ceramiczną** | 1 szt. | Pozycja, prędkość rzeczywista, kurs (NMEA 9600 bps) |
| 09 | **Adapter zaciskowy** | **Grove 4-pin → terminal śrubowy ARK** | 1 szt. | Bezlutowe wyprowadzenie sygnałów zewnętrznych |

### 3. Zasilanie, Okablowanie i Mechanika:
| Lp. | Komponent | Model / Oznaczenie | Ilość | Rola / Zastosowanie |
|---|---|---|:---:|---|
| 10 | **Powerbank pokładowy** | **everActive EB-22QB 20 000 mAh (20W PD/QC)** | 1 szt. | Niezależne zasilanie ESP32-S3 i smartfona 5G |
| 11 | **Kable USB-C** | 1x kątowy 90° 30 cm (telefon) + 1x prosty 30 cm (ESP) | 2 szt. | Zasilanie urządzeń wewnątrz kokpitu bez naprężeń |
| 12 | **Wiązki sygnałowe** | Grove 4-pin F-F (5 szt.) + Grove→Dupont + Dupont F-F | 1 kpl. | Szyna Grove I2C, sygnały UART i CRSF |
| 13 | **Przedłużacze JR** | 30 cm 22AWG płaskie MSP | 2 szt. | Połączenie serwa skrętu i regulatora ESC do PCA9685 |
| 14 | **Obudowa ochronna** | **Pawbol S-Box 216C IP65 (120×80×50 mm)** | 1 szt. | Ochrona elektroniki przed kurzem, błotem i wodą |
| 15 | **Dławiki i montaż** | Dławiki PG7 IP68 + Rzep **3M Dual Lock SJ3550** | 1 kpl. | Szczelne przejścia kablowe i antywibracyjny montaż |
| 16 | **Stanowisko 5G FPV** | SmallRig 2164 + uchwyt telefonu + cooler Peltiera | 1 kpl. | Montaż smartfona na MT12 ze stałym chłodzeniem |

---

## 🔌 Schemat Połączeń Śrubowych (ESP32-S3 Terminal Shield 44-pin)

Wszystkie połączenia peryferyjne realizowane są na **złączach śrubowych ARK Terminal Shielda**:

| Peryferium / Moduł | Zacisk Shielda | Pin ESP32-S3 | Sygnał / Standard | Opis i Uwagi |
|---|---|---|---|---|
| **Grove TCA9548A Hub** | `SDA` | `GPIO 8` | I2C Data | Linia danych I2C (wspólna dla TCA9548A, PCA, IMU) |
| **Grove TCA9548A Hub** | `SCL` | `GPIO 9` | I2C Clock | Linia zegara I2C Fast Mode (400 kHz) |
| **Grove TCA9548A Hub** | `3V3` / `5V` | `3.3V` lub `5V` | VCC | Zasilanie logiki modułów Grove |
| **Wspólna Masa** | `GND` | `GND` | Ground | **Kluczowa wspólna masa** (ESP, PCA, ESC, BEC) |
| **Grove Air530 GPS** | `18` | `GPIO 18` (RX1) | UART RX <- GPS TX | Odbiór ramek NMEA (9600 bps) |
| **Grove Air530 GPS** | `17` | `GPIO 17` (TX1) | UART TX -> GPS RX | Konfiguracja modułu GPS (opcjonalna) |
| **Odbiornik ER3C-i / ER5C-i** | `15` | `GPIO 15` (RX2) | CRSF RX <- Odb. TX | Odbiór sterowania CRSF z MT12 (420 000 bps) |
| **Odbiornik ER3C-i / ER5C-i** | `16` | `GPIO 16` (TX2) | CRSF TX -> Odb. RX | Zwrotna telemetria CRSF na ekran aparatury |
| **Dzielnik Baterii (VBAT)** | `1` | `GPIO 1` (ADC1) | Analog IN (0-3.3V) | Pomiar pakietu 4S LiPo (dzielnik R1=10k, R2=2.2k) |
| **Zasilanie ESP32-S3** | `USB-C` | Port USB ESP32 | 5V DC (Power Delivery) | Bezpośrednio z powerbanku everActive EB-22QB |

> [!CAUTION]
> **Ochrona pamięci Octal Flash/PSRAM w ESP32-S3:**
> Płytki N8R8 i N16R8 wykorzystują piny `GPIO 33, 34, 35, 36, 37` do wewnętrznej magistrali szybkiej pamięci SPI. **Kategorycznie zabrania się podłączania czegokolwiek do tych pinów!** Pomiar baterii został bezpiecznie przeniesiony na kanał `ADC1_CH0` (`GPIO 1`).

---

## 🎛️ Podłączenie Modułów na Szynie Grove I2C

```
[ ESP32-S3 DevKit ]
  (GPIO 8: SDA) ────┐
  (GPIO 9: SCL) ────┼───> [ Grove TCA9548A I2C Hub (0x70) ]
  (3V3 / GND)   ────┘          │
                               ├── Port 0: [ Grove PCA9685 (0x40) ]
                               │             ├── CH0: Serwo skrętu ARRMA (JR)
                               │             └── CH1: Regulator ESC Spektrum Firma (JR)
                               │
                               └── Port 1: [ Grove LSM6DS3 IMU (0x6A) ]
                                             └── Sztuczny horyzont, przeciążenia G
```

1. **Grove PCA9685:**
   - Wpięty przewodem Grove do **Portu 0** w hubie TCA9548A.
   - Kanał 0 (`CH0`): Serwo skrętu przedniej osi (przewód JR 3-pin).
   - Kanał 1 (`CH1`): Regulator ESC Spektrum Firma (przewód JR 3-pin).
   - Szyna zasilania serw (`V+` na PCA9685) zasilana jest z wbudowanego BEC regulatora ESC lub dedykowanego BEC 6.0V/7.4V.
2. **Grove LSM6DS3:**
   - Wpięty przewodem Grove do **Portu 1** w hubie TCA9548A.
   - Zamontowany płasko i sztywno na płycie podwozia za pomocą taśmy 3M Dual Lock.
3. **Grove GPS Air530:**
   - Podłączony do portu UART1 (`GPIO 18 RX`, `GPIO 17 TX`) za pomocą przejściówki Grove → Dupont / Terminal.
   - Ceramiczna antena GPS skierowana poziomo ku górze.

---

## ⚙️ Konfiguracja Odbiorników RadioMaster ER3C-i / ER5C-i (CRSF)

1. Połącz się ze smartfona z punktem dostępowym Wi-Fi odbiornika (hasło domyślne: `expresslrs`).
2. Przejdź w przeglądarce pod adres `http://10.0.0.1`.
3. W zakładce **Model / Hardware**:
   - Skonfiguruj wyjście szeregowe na tryb **CRSF**:
     * `Pin TX` (CRSF Out) $\rightarrow$ podłącz do `GPIO 15` (RX2) w shieldzie ESP32-S3.
     * `Pin RX` (CRSF In / Telemetry) $\rightarrow$ podłącz do `GPIO 16` (TX2) w shieldzie ESP32-S3.
     * Baudrate: `420000` (lub `115200` jeśli preferowane).
4. Po zbindowaniu z aparaturą RadioMaster MT12 wejdź w menu aparatury:
   - *Model Setup* $\rightarrow$ *Telemetry* $\rightarrow$ *Discover new sensors*.
   - Natychmiast pojawią się sensory pokładowe:
     * `RxBt` (Napięcie głównego pakietu napędowego 4S)
     * `GPS` (Koordynaty, prędkość, kurs)
     * `Ptch`, `Roll`, `Yaw` (Kąty pochylenia z Grove LSM6DS3)
     * `RQly` (Jakość sygnału radiowego)

---

## 🔋 Zasilanie z Powerbanku everActive EB-22QB

1. **Port USB-C 1:** Podłączony kablem USB-C (30 cm) do złącza zasilania płytki **ESP32-S3 DevKit**.
2. **Port USB-C 2:** Podłączony kablem kątowym 90° (30 cm) do smartfona FPV z kartą 5G T-Mobile.
3. **Masa układu:** Czarny przewód masy z regulatora ESC / złącza zasilania serw musi być wpięty w zacisk `GND` Terminal Shielda ARK, aby zapewnić wspólny punkt odniesienia sygnałów PWM i UART.

---

## 🎮 Przypisanie Kanałów w Aparaturze MT12

| Kanał | Nazwa | Funkcja | Zakres sygnału | Stan Fail-Safe / DISARM |
|---|---|---|---|---|
| **CH 1** | Steering | Skręt kół (CH0 PCA9685) | 1000 µs – 1500 µs – 2000 µs | `1500 µs` (Koła na wprost) |
| **CH 2** | Throttle | Gaz / Hamulec (CH1 PCA9685) | 1000 µs – 1500 µs – 2000 µs | `1500 µs` (Hamulec neutralny) |
| **CH 5** | AUX 1 | Uzbrojenie (ARM Switch) | > 1350 µs = ARMED, < 1350 µs = STOP | `1000 µs` (Blokada napędu) |
| **CH 6** | AUX 2 | Przełącznik Muxera (Radio / 5G) | < 1300 µs: RF, 1300-1700: AUTO, > 1700 µs: 5G | Tryb AUTO (Fallback na 5G) |

---

## 🛠️ Wymagane Biblioteki w Arduino IDE

- **Adafruit PWM Servo Driver Library** (do sterownika PCA9685)
- **TinyGPSPlus** (do dekodowania strumienia NMEA z Grove Air530)
*(Sterownik Grove LSM6DS3 oraz obsługa huba Grove TCA9548A są wbudowane bezpośrednio w firmware i nie wymagają instalowania zewnętrznych bibliotek!)*
