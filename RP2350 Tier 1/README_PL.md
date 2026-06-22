# RCSIM - RP2350 USB to PPM Bridge (Tier 1)

Oprogramowanie układowe (firmware) oraz specyfikacja techniczna dla mikrokontrolera **Seeed Studio XIAO RP2350** realizującego funkcję mostka USB-szeregowy na sygnał PPM (Pulse Position Modulation). Tryb ten jest wykorzystywany w systemie RCSIM do bezpośredniego sterowania pojazdem za pomocą portu trenera aparatury (np. RadioMaster MT12) podłączonego pod stację GCS.

## Specyfikacja techniczna

- **Mikrokontroler:** Seeed Studio XIAO RP2350
- **Wyjście sygnału PPM:** Pin D8 (GPIO 2) — 5V-tolerant na płytce XIAO
- **Liczba kanałów:** 8
- **Długość ramki PPM:** 22.5 ms (22500 us)
- **Impuls synchronizujący:** 300 us
- **Szybkość komunikacji USB (Serial):** 115200 bps
- **Format danych:** Tekstowy CSV: `ch1,ch2,ch3,ch4,ch5,ch6,ch7,ch8\n` (wartości w zakresie 800 - 2200 us, środek 1500 us)

## Architektura dwurdzeniowa (Dual-Core)

W celu wyeliminowania jitteru sygnału PPM wywoływanego przez operacje I/O, firmware wykorzystuje dwa rdzenie mikrokontrolera RP2350:
- **Core 0 (Rdzeń główny):** Odpowiada za odbiór pakietów CSV przez port USB CDC, ich walidację, parsowanie oraz obsługę mechanizmu Failsafe / E-STOP.
- **Core 1 (Rdzeń pomocniczy):** Zajmuje się wyłącznie precyzyjnym generowaniem sygnału PPM w pętli opóźnień czasowych przy wyłączonych przerwaniach systemowych. Zapobiega to jakimkolwiek zakłóceniom w taktowaniu sygnału.

## Bezpieczeństwo i Failsafe

- **Hardware Signal Kill (Failsafe):** Jeśli mikrokontroler nie otrzyma poprawnej ramki sterującej w ciągu **500 ms**, automatycznie wchodzi w tryb Failsafe. Pin wyjściowy PPM jest przełączany w tryb wysokiej impedancji (`Hi-Z` / `INPUT`), co powoduje, że aparatura wykrywa brak sygnału ("Signal Lost") i natychmiast zatrzymuje pojazd.
- **Wbudowane komendy:** 
  - `ESTOP\n` — Natychmiastowe odcięcie sygnału PPM (Emergency Stop).
  - `ARM\n` — Uzbrojenie i zezwolenie na transmisję PPM po usunięciu blokady E-STOP.

## Instrukcja wdrożenia (PlatformIO)

Aby skompilować oprogramowanie, stwórz projekt w środowisku PlatformIO z następującym plikiem `platformio.ini`:

```ini
[env:seeed_xiao_rp2350]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = seeed_xiao_rp2350
framework = arduino
board_build.core = earlephilhower
monitor_speed = 115200
```

Umieść plik `main.cpp` w folderze `src/` i wgraj oprogramowanie na płytkę.
