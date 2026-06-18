# 🎛️ RCSIM Firmware Subsystem (Tiers 1, 2, 6 & Lights)

Projekt ten stanowi podsystem oprogramowania układowego (Firmware) dla platformy **RCSIM (RC Simulator / Ground Control Station)**. Oprogramowanie obsługuje różne poziomy (Tiers) integracji sprzętowej – od prostych konwerterów sygnałów po zaawansowane koncentratory bezprzewodowe z wizją oraz sprzętowe układy Watchdog nadzorujące komputery jednopłytkowe (Raspberry Pi 5).

---

## 📐 Architektura Systemu

Poniższy schemat przedstawia strukturę przepływu sygnałów i kontroli w zależności od wybranego trybu pracy podsystemu firmware:

```
                  +---------------------------+
                  |    Stacja Naziemna GCS    |
                  |     (Aplikacja PC / AI)   |
                  +-------------+-------------+
                                |
         +----------------------+----------------------+
         | (Serial USB)                                | (WiFi UDP / Video Stream)
         v                                             v
+--------+------------------+                 +--------+------------------+
|      [Tier 1 Bridge]      |                 |    [Tier 2 Wireless Hub] |
|   Arduino Serial-to-PPM  |                 |      ESP32 Controller      |
+--------+------------------+                 +--------+------------------+
         |                                             |
         | (Sygnał PPM)                                | (Magistrala I2C)
         v                                             v
+--------+------------------+                 +--------+------------------+
|      Aparatura RC         |                 |    Sterownik PCA9685     |
|   (np. RadioMaster MT12)  |                 |    (Serwa, Napęd, Lux)   |
+--------+------------------+                 +--------+---------+--------+
         |                                                       
         | (Sygnał PPM)                                          
         |                                                       




+--------+-------------------------------------------------------+--------+
|                      [Tier 6 Watchdog & MUX]                            |
|             Koprocesor Bezpieczeństwa ESP32 (RCSIM HAT)                 |
+------------------------------------+------------------------------------+
                                     |
                         (Heartbeat) | (Status Override)
                                     v
                  +------------------+------------------------+
                  |             Raspberry Pi 5                |
                  |      (Jednostka Autonomiczna / SLAM)      |
                  +-------------------------------------------+
```

---

## 🗂️ Opis Modułów (Tiers)

### 📟 Tier 1: Konwerter Serial na PPM (Arduino)
*Lokalizacja:* `[Arduino Tier 1](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/Arduino%20Tier%201)`

Mostek komunikacyjny realizujący konwersję komend szeregowych z komputera GCS na standardowy sygnał PPM (Pulse Position Modulation).
*   **Komunikacja:** Port szeregowy USB (`115200 bps`), format danych: `ch1,ch2,...,ch8\n`.
*   **Fizyczny Failsafe (Signal Kill):** Brak poprawnych ramek przez ponad `500 ms` wyłącza przerwania Timera1 i przełącza pin PPM (`D10`) w stan wysokiej impedancji (`INPUT`). Aparatura RC natychmiast wykrywa utratę sygnału (Trainer Lost) i aktywuje własne procedury bezpieczeństwa.
*   **Sprzętowy E-STOP:** Obsługa natychmiastowych komend tekstowych `ESTOP` (odcięcie sygnału PPM i blokada sterowania) oraz `ARM` (odblokowanie).
*   **Sygnalizacja:** Dioda LED miga z częstotliwością ok. 5 Hz podczas poprawnej pracy. W stanie Failsafe/E-STOP dioda gaśnie.

---

### 🌐 Tier 2: Bezprzewodowy Hub Sterowania (ESP32)
*Lokalizacja:* `[ESP32 Tier 2](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/ESP32%20Tier%202)`

Zaawansowany bezprzewodowy koncentrator sterowania, wizji i telemetrii oparty na mikrokontrolerach ESP32 (ESP32-CAM, ESP32-Wrover).
*   **Autokalibracja PCA9685 (`calibratePCA9685`):** Sprzężenie zwrotne korygujące częstotliwość wewnętrznego oscylatora PCA9685. Testowy sygnał 1500 us z kanału 15 PCA9685 jest wpinany do GPIO 12 na ESP32. Mikrokontroler mierzy impuls i dostosowuje oscylator do momentu, aż błąd spadnie poniżej 3 us.
*   **Strumieniowanie Wizji:** Klatki MJPEG przesyłane przez HTTP na porcie `81`. Wideo obsługiwane asynchronicznie w osobnym wątku FreeRTOS przypisanym do rdzenia 0.
*   **Sterowanie UDP:** Odbiór komend sterujących na porcie `12345`.
*   **Telemetria IMU:** Odczyt danych z sensora MPU6050 i wysyłka w postaci JSON przez UDP na port `12347`.
*   **Zabezpieczenia:** Watchdog sieciowy (ponowne łączenie po utracie WiFi) oraz Watchdog sterowania (failsafe na 1500 us po 500 ms braku danych UDP).

---

### 💡 Arduino Lights: Inteligentny Sterownik Oświetlenia RC
*Lokalizacja:* `[Arduino Lights](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/Arduino%20Lights)`

Dedykowany sterownik oświetlenia LED dla modeli RC dekodujący sygnały PWM z odbiornika.
*   **Tryb Pełny:** Reaguje na CH1 (kierunkowskazy z opóźnieniem histerezy 300 ms), CH2 (stop po zatrzymaniu lub hamowaniu, wsteczny automatyczny), CH3 (włączanie reflektorów i świateł dachowych).
*   **Tryb Jednokanałowy (Muxed Fallback):** Aktywowany w przypadku podłączenia tylko CH3. Umożliwia wybór stanu świateł przełącznikiem.
*   **PCINT (Pin Change Interrupt):** Nieblokujący odczyt szerokości impulsów z odbiornika na pinach D8, D9, D10 bez użycia spowalniającej funkcji `pulseIn()`.
*   **Ochrona Active-Low:** Piny przełączane w tryb `INPUT` (wysoka impedancja) zamiast stanu `HIGH` przy wyłączaniu LED, co zapobiega powstawaniu prądów wstecznych.
*   **Failsafe:** Brak sygnału przez 500 ms uruchamia automatycznie światła awaryjne (miganie obu kierunkowskazów).

---

### 🛡️ Tier 6: Koprocesor Watchdoga i Muxera dla RPi (ESP32)
*Lokalizacja:* `[ESP32 Tier 6 Watchdog for RPi](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/ESP32%20Tier%206%20Watchdog%20for%20RPi)`

Nadrzędny sterownik bezpieczeństwa (Supervisor) nadzorujący pracę komputera Raspberry Pi 5.
*   **Detekcja Heartbeat:** Monitorowanie sygnału z RPi 5 na pinie `PIN_HEARTBEAT` (GPIO 4). Brak impulsu przez 1000 ms oznacza zawieszenie komputera i wyzwala tryb Failsafe.
*   **Manual Override (SBUS):** Odczyt sygnału SBUS z odbiornika na pinie `PIN_SBUS_RX` (GPIO 15) przy użyciu UART2 z wykorzystaniem wbudowanej w ESP32 inwersji sprzętowej (brak potrzeby dodatkowych tranzystorów).
*   **Ochrona magistrali I2C:** Zaimplementowana maszyna stanów (`SystemState`) wysyła komendy awaryjne do PCA9685 dokładnie raz podczas zmiany stanu, co minimalizuje kolizje na magistrali I2C współdzielonej z RPi.
*   **Sygnalizacja stanu (`PIN_OVR_STATUS`):** Pin GPIO 5 informuje Raspberry Pi o przejęciu kontroli (`LOW` - tryb ręczny/failsafe, `HIGH` - kontrola RPi).

---

## ⚡ Wskazówki Dotyczące Zasilania i Bezpieczeństwa

> [!IMPORTANT]
> **Zasilanie ESP32:** Układy ESP32-CAM oraz ESP32-Wrover pobierają znaczny prąd podczas transmisji WiFi i pracy kamery (szpilki prądowe do 500mA). Muszą być zasilane z dedykowanego układu BEC 5V o wydajności minimum 1.5A–2A.
> **Separacja zasilania serw:** Serwomechanizmy podłączone do PCA9685 muszą być zasilane z zewnętrznego akumulatora podłączonego do niebieskiego zacisku śrubowego na płytce PCA9685 (V+). Podłączenie zasilania serw bezpośrednio do linii 5V ESP32 spowoduje natychmiastowy reset mikrokontrolera.

---

## 🔧 Konfiguracja i Uruchomienie

1.  **Wybór modułu:** Przejdź do folderu odpowiadającego Twojej konfiguracji sprzętowej.
2.  **Kompilacja:** Otwórz wybrany projekt w Arduino IDE lub VS Code z rozszerzeniem PlatformIO.
3.  **Wgranie kodu:** Upewnij się, że posiadasz zainstalowane wymagane biblioteki (`PPMEncoder`, `Bolder Flight Systems SBUS`, `Adafruit PWM Servo Driver`). Wgraj program na mikrokontroler.
4.  **Autokalibracja (Tier 2):** Aby przeprowadzić autokalibrację PCA9685, wykonaj połączenie fizyczne z kanału 15 PCA9685 do GPIO 12 ESP32 przed uruchomieniem zasilania.

---

## 📄 Licencja

Projekt dystrybuowany jest na licencji **MIT**. Szczegóły znajdują się w pliku [LICENSE](file:///c:/Users/Mateusz/Desktop/RCSIM27.04monacoSLAM/RCSIM_PC/pc_app/ESP32Arduino/RCSIM_Arduino_ESP32/LICENSE).
