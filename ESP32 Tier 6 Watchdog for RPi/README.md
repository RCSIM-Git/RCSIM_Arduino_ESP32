# RCSIM - Sprzętowy Watchdog i Muxer dla ESP32 (RCSIM HAT V1.0)

Koprocesor nadzorujący (Supervisor Controller) zrealizowany na bazie mikrokontrolera ESP32, odpowiedzialny za zarządzanie stanem bezpieczeństwa, detekcję zawieszenia komputera głównego (Raspberry Pi 5) oraz obsługę manualnego przejęcia kontroli (Manual Override).

## Główne Funkcje

1. **Detekcja Heartbeat (Watchdog):**
   * Stale monitoruje impulsy (Heartbeat) z Raspberry Pi 5 na pinie `PIN_HEARTBEAT` (GPIO 4) za pomocą przerwania.
   * Brak zmiany stanu pinu przez ponad `1000 ms` jest interpretowany jako zawieszenie systemu Linux i uruchamia procedurę awaryjną.

2. **Manual Override (Natywny SBUS):**
   * Bezpośredni odczyt sygnału SBUS z odbiornika RC na pinie `PIN_SBUS_RX` (GPIO 15) przy użyciu sprzętowego portu UART2.
   * Dzięki wbudowanym inwerterom sprzętowym w ESP32, sygnał SBUS (odwrócony UART) jest czytany bezpośrednio **bez konieczności stosowania zewnętrznego inwertera tranzystorowego**.
   * Przełączenie dedykowanego kanału (domyślnie kanał 5 > 1600) natychmiast przekazuje kontrolę nad pojazdem operatorowi.

3. **Double-Failsafe (Ochrona przed utratą zasięgu):**
   * System nie tylko sprawdza poprawność działania RPi5, ale również stan odbiornika RC.
   * Jeśli operator przełączy pojazd w tryb manualny, a nastąpi utrata sygnału RC (failsafe zgłoszony przez aparaturę/odbiornik), układ natychmiast wymusza stan bezpieczny (`STATE_FAILSAFE`), zapobiegając ucieczce modelu.

4. **Ochrona szyny I2C (Brak spamu PCA9685):**
   * Zaimplementowano pełną maszynę stanów (`SystemState`).
   * Zamiast ciągłego wysyłania komend I2C w pętli (co mogłoby zablokować szynę lub generować zakłócenia), sygnały bezpieczeństwa (`applySafeState`) oraz zmiany trybów są wysyłane do PCA9685 **dokładnie raz** podczas przejścia stanu.
   * Ciągłe nadawanie wartości PWM odbywa się wyłącznie w trybie manualnym (`STATE_MANUAL_OVR`), gdy rzeczywiście zmieniają się pozycje drążków aparatury.

5. **Sygnalizacja zwrotna Muxera (`PIN_OVR_STATUS`):**
   * Pin `PIN_OVR_STATUS` (GPIO 5) informuje Raspberry Pi 5 o aktualnym dysponencie szyny:
     * `HIGH` - Tryb normalny, Raspberry Pi kontroluje pojazd.
     * `LOW` - Tryb przejęcia kontroli (Manual/Failsafe), RPi5 powinno zaprzestać prób wysyłania komend na szynę oraz dostosować swoje zachowanie (np. zatrzymać nagrywanie logów AI).

## Wymagania biblioteczne

Do kompilacji szkicu wymagana jest biblioteka:
* **bolderflight/Bolder Flight Systems SBUS** (dostępna w bibliotekach PlatformIO/Arduino)
* **Adafruit PWM Servo Driver Library** (do obsługi układu PCA9685)
