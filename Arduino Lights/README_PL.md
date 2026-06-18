# RCSIM - Inteligentny Sterownik Oświetlenia RC dla Arduino (v1.0)

Projekt ten stanowi oprogramowanie dedykowanego sterownika oświetlenia LED dla modeli pojazdów RC, zrealizowany na bazie mikrokontrolera Arduino (np. Nano/Uno z układem ATmega328P). System automatycznie dekoduje sygnały PWM z odbiornika RC i dostosowuje stany świateł pojazdu (reflektory, stop, wsteczny, kierunkowskazy).

## Funkcje Kluczowe

1. **Dwa Tryby Pracy (Autodetekcja):**
   * **Tryb Pełny (Wielokanałowy):** Podłączone są sygnały CH1 (Skręt), CH2 (Gaz) i CH3 (Przełącznik pomocniczy). Światła reagują w pełni dynamicznie na zachowanie pojazdu.
   * **Tryb Jednokanałowy (Muxed Fallback):** Podłączony jest wyłącznie kanał CH3. Umożliwia to podstawową kontrolę w uproszczonych instalacjach: CH3 steruje kierunkowskazami (pozycja niska = lewy, wysoka = prawy, środkowa = wyłączone), a reflektory i światła tylne świecą stale.

2. **Nieblokujący Odczyt Przerwaniami (PCINT):**
   * Pomiar szerokości impulsów PWM z odbiornika odbywa się za pomocą rejestrów przerwań sprzętowych `PCINT0` (piny D8, D9, D10).
   * Dzięki temu mikrokontroler nie blokuje pętli głównej powolnymi wywołaniami `pulseIn()`, co gwarantuje płynność działania i natychmiastową reakcję świateł.

3. **Zabezpieczenie Napięciowe (Masa Active-Low):**
   * Diody LED sterowane są niskim stanem napięcia (Active-Low).
   * W celu wyłączenia diody, pin mikrokontrolera nie jest ustawiany w stan `HIGH` (co mogłoby wywołać prąd wsteczny z szyny zasilania 3.3V do 5V Arduino), lecz przechodzi w **stan wysokiej impedancji (`INPUT`)**. Chroni to elektronikę odbiornika oraz regulatora ESC.

4. **Logika Świateł w Trybie Pełnym:**
   * **Reflektory przednie (D2) & Światła dodatkowe (D7):** Kontrolowane przez 3-pozycyjny przełącznik CH3 (dół = wyłączone, środek = reflektory ON, góra = wszystko ON).
   * **Światła STOP (D3):** Świecą pełną mocą przy hamowaniu lub w pozycji neutralnej drążka gazu (jako światła pozycyjne). Wyłączają się podczas jazdy do przodu.
   * **Światła cofania (D4):** Włączają się automatycznie po wykryciu wstecznego wychylenia drążka przepustnicy.
   * **Kierunkowskazy automatyczne (D5 / D6):** Uruchamiają się po wychyleniu kierownicy (CH1). Posiadają histerezę czasową (`turn_signal_delay_threshold = 300 ms`), co zapobiega miganiu kierunkowskazów podczas szybkich kontr i jazdy po łukach toru.

5. **Tryb Awaryjny (Failsafe):**
   * Brak sygnałów z odbiornika przez ponad `500 ms` wyłącza reflektory główne i aktywuje **światła awaryjne** (jednoczesne miganie lewego i prawego kierunkowskazu), co sygnalizuje utratę zasięgu aparatury lub uszkodzenie instalacji.

## Pinout Sterownika (Arduino Nano)

| Funkcja | Pin Arduino | Opis połączenia |
|---|---|---|
| **Wejście CH1** | **D8** | Sygnał PWM Skrętu z odbiornika RC |
| **Wejście CH2** | **D9** | Sygnał PWM Gaz/Hamulec z odbiornika |
| **Wejście CH3** | **D10** | Sygnał PWM Aux z odbiornika |
| **Reflektory przednie** | **D2** | Sterowanie masą LED przednich |
| **Światła STOP / Tył** | **D3** | Sterowanie masą LED tylnych czerwonych |
| **Światła cofania** | **D4** | Sterowanie masą LED cofania |
| **Kierunkowskaz lewy** | **D5** | Sterowanie masą LED lewych migaczy |
| **Kierunkowskaz prawy**| **D6** | Sterowanie masą LED prawych migaczy |
| **Światła dachowe/Aux** | **D7** | Sterowanie masą LED dodatkowych/dachowych |

## Sposób Uruchomienia

1. Wgraj program `RCSIM_Lights.ino` na płytkę Arduino Nano przy użyciu Arduino IDE.
2. Połącz wejścia D8, D9, D10 z odpowiednimi kanałami odbiornika RC (użyj kabli ze wspólną masą i zasilaniem z BEC/odbiornika).
3. Podłącz diody LED poprzez rezystory ograniczające prąd do zasilania (katody diod podłącz do pinów D2-D7 Arduino).
4. Po uruchomieniu system dokona automatycznej kalibracji punktu neutralnego dla kierownicy i przepustnicy (upewnij się, że drążki aparatury są w pozycji neutralnej przez pierwszą sekundę po włączeniu zasilania).


+-------------------+
                  |   Odbiornik RC    |
                  |                   |
                  | CH1  CH2   CH3    |
                  +-|-----|-----|-----+
                    |     |     |
                 [KabelY][KabelY]       
                  /   \   /   \ 
                 /     \ /     \        (Sygnał + Masa)
          [Serwo]    [ESC]      +------> Do Arduino CH3 (D10 + GND)
            |          |                 oraz zasilanie Arduino (5V)
            |          |
            |          +---------------> Do Arduino CH2 (D9 Sygnał + GND)
            +--------------------------> Do Arduino CH1 (D8 Sygnał + GND)