# RCSIM - Bezprzewodowy Hub Sterowania dla ESP32 z Autokalibracją PCA9685 (V3.2)

Projekt ten stanowi bezprzewodowy koncentrator sterowania i telemetrii (Hub), który łączy w sobie obsługę strumieniowania wideo (MJPEG), telemetrię z czujnika IMU (MPU6050) oraz sterowanie serwami przez sieć za pomocą sterownika PCA9685 z  funkcją automatycznej kalibracji sprzętowej.

## Funkcje Kluczowe

1. **Autokalibracja Sprzętowa PCA9685 (`calibratePCA9685`):**
   * Układy PCA9685 posiadają wewnętrzne oscylatory o częstotliwości nominalnej 25 MHz, jednak wykazują one spory rozrzut produkcyjny i dryft termiczny. Powoduje to, że rzeczywisty czas trwania impulsu PWM różni się od zadeklowanego.
   * Hub wykorzystuje pętlę sprzężenia zwrotnego: kanał 15 (`CALIBRATION_CH`) z PCA9685 wysyła testowy sygnał 1500 us bezpośrednio do pinu GPIO 12 (`CALIBRATION_PIN`) mikrokontrolera ESP32.
   * ESP32 mierzy czas trwania impulsu wysokiego za pomocą `pulseIn()` i koryguje częstotliwość wewnętrznego oscylatora układu PCA9685 w pętli (do 15 prób), dopóki błąd nie spadnie poniżej 3 mikrosekund.
   * Wynikowa, skalibrowana częstotliwość jest zapisywana za pomocą `pca.setOscillatorFrequency(currentFreq)`.

2. **Bezprzewodowe Sterowanie UDP:**
   * Odbiór komend sterujących w postaci tekstowej (np. `"1500,1500,1000,..."` rozdzielone przecinkami) na porcie `12345`.
   * Przeliczanie mikrosekund (1000-2000 us) na wartości rejestrów PCA9685 bez spowalniających operacji zmiennoprzecinkowych.
   * Zabezpieczenie przed uszkodzonymi pakietami (odrzucanie wartości spoza zakresu 800 - 2200 us).

3. **Strumień Wideo (ESP32-CAM):**
   * Strumieniowanie obrazu w standardzie MJPEG przez HTTP na porcie `81`.
   * Obsługa profili płytek: AI Thinker, ESP32-Wrover-Dev oraz LilyGo T-SimCam S3.
   * Obsługa wątkowości w systemie FreeRTOS (strumień wideo działa w osobnym asynchronicznym wątku przypiętym do rdzenia 0).

4. **Telemetria IMU:**
   * Odczyt danych z czujnika MPU6050 (akcelerometr + żyroskop) po szynie I2C.
   * Wysyłanie telemetrii jako pakietów JSON przez UDP do komputera PC na port `12347`.

5. **Zabezpieczenie Failsafe:**
   * Watchdog sieciowy: wykrycie utraty połączenia WiFi (dłużej niż 5s) inicjuje próbę automatycznej ponownej konfiguracji i połączenia z AP.
   * Watchdog sterowania: brak pakietów UDP przez ponad `500 ms` aktywuje funkcję `triggerFailsafe()`, ustawiając wszystkie 16 kanałów w pozycję neutralną (1500 us).

## Schemat Połączeń (Wymagany do Autokalibracji)

Aby funkcja autokalibracji działała poprawnie, należy fizycznie połączyć:
* **PCA9685 Kanał 15 (Sygnał PWM - żółty/biały przewód)** -> **ESP32 GPIO 12**
* Wspólna masa (GND) musi być podłączona pomiędzy ESP32, PCA9685 oraz zasilaniem serw.

## Konfiguracja I2C

Urządzenia peryferyjne podłączone są do dedykowanych pinów I2C zależnie od profilu płytki:
* Np. dla profilu WROVER: `SDA = 13`, `SCL = 14`.
* Hub posiada wbudowany skaner szyny I2C (`i2c_scan()`), który na starcie systemu wypisuje w terminalu szeregowym (115200 bps) wszystkie wykryte urządzenia (szczególnie poszukując adresów `0x40` dla PCA9685 oraz `0x68` dla IMU).
