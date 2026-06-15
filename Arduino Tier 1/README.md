# RCSIM - Konwerter Serial na PPM dla Arduino (v3.0 - High Stability)

Ten projekt stanowi oprogramowanie mostka komunikacyjnego (Bridge) dla platformy Arduino (np. Nano/Uno z mikrokontrolerem ATmega328P), realizującego konwersję komend szeregowych z komputera stacji naziemnej (GCS) na standardowy sygnał PPM (Pulse Position Modulation) z wbudowanym sprzętowym wyłącznikiem bezpieczeństwa (Signal Kill).

## Główne Funkcje

1. **Konwersja Serial-to-PPM (8 kanałów):**
   * Odbiór danych z portu szeregowego USB (Baudrate: `115200 bps`) w formacie tekstowym `"ch1,ch2,...,ch8\n"`.
   * Przetwarzanie i kodowanie wartości mikrosekund (zakres 1000 - 2000 us) na zespolony sygnał PPM o polaryzacji dodatniej, generowany na pinie `PPM_OUTPUT_PIN` (D10).
   * Generowanie PPM opiera się o stabilne przerwania sprzętowe (Timer1 dla AVR).

2. **Krytyczny Failsafe (Fizyczne odcięcie sygnału):**
   * W przypadku braku poprawnych ramek sterowania z komputera PC przez czas przekraczający `500 ms` (np. zawieszenie aplikacji GCS lub odłączenie kabla USB), układ wchodzi w stan awaryjny.
   * W przeciwieństwie do prostych rozwiązań wysyłających wartości neutralne, ten moduł realizuje **Signal Kill**: wyłącza przerwania Timera1 oraz przełącza pin `PPM_OUTPUT_PIN` w tryb wejścia (`INPUT`). 
   * Aparatura RC (np. RadioMaster MT12) podpięta przez port trenera natychmiast wykrywa całkowity brak sygnału PPM i aktywuje własną procedurę awaryjną (np. odcięcie gazu).

3. **Sprzętowy Wyłącznik Awaryjny (E-STOP & ARM):**
   * Układ reaguje na natychmiastowe komendy tekstowe z GCS wysyłane poza standardowym strumieniem kanałów:
     * `ESTOP` - Natychmiastowe zatrzymanie generowania PPM (fizyczne odcięcie pinu wyjściowego). System ignoruje wszelkie kolejne pakiety sterujące, dopóki nie zostanie uzbrojony.
     * `ARM` - Przywrócenie możliwości działania po zaistnieniu blokady awaryjnej.

4. **Sygnalizacja Stanu Diodą LED:**
   * **Brak połączenia / Failsafe / E-Stop:** Dioda LED wbudowana w płytkę (`LED_PIN`) jest całkowicie wyłączona.
   * **Normalna praca:** Dioda LED miga szybko i wyraźnie w tempie pracy systemu (~5 Hz), potwierdzając aktywny przepływ prawidłowych danych i stan uzbrojenia.

## Wymagania Sprzętowe i Biblioteki

* **Mikrokontroler:** Arduino Nano / Uno (ATmega328P).
* **Biblioteka:** `PPMEncoder` (odpowiedzialna za sprzętowe generowanie PPM na Timer1).
* **Połączenie:** Pin D10 (Wyjście PPM) -> Wejście portu trenera (Jack 3.5mm w aparaturze RC).

## Sposób Użycia

1. Wgraj szkic `Arduino.ino` na płytkę Arduino przy użyciu Arduino IDE.
2. Podłącz Arduino do komputera przez port USB.
3. W aplikacji stacji naziemnej (GCS) wybierz tryb połączenia **Arduino (Serial)**, wskaż właściwy port COM oraz prędkość **115200 bps**.
4. Podłącz pin D10 i masę (GND) z Arduino do wejścia PPM aparatury (port trenera/DSC).
