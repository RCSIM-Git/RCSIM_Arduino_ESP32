# 🎮 EdgeTX — CRSF Trainer przez USB-VCP + Pełny Dupleks Telemetrii

### Zmodyfikowany Firmware EdgeTX dla **RadioMaster MT12** oraz **RadioMaster Pocket**
*Oparty na Pull Requeście EdgeTX **#7630** (`feat/crsf-trainer-over-usb-vcp`, commit `e5784ee5`) z autorskim patchem `telemetrySetMirrorCb` realizującym pełny zwrot telemetrii do PC.*

---

## 🚀 Przegląd i Kluczowe Korzyści

Oprogramowanie zamienia aparaturę RadioMaster w **dwukierunkowy transceiver CRSF o zerowej latencji**, wykorzystujący pojedynczy fabryczny kabel USB-C:

1. **Bezpośrednie sterowanie z PC (RX 100–250 Hz):**
   Komputer (aplikacja **RCSIM-GCS**) przesyła standardowe ramki CRSF (kanały 1–16) bezpośrednio po wirtualnym porcie szeregowym USB-C (VCP) do miksera EdgeTX i wewnętrznego modułu nadawczego ExpressLRS (ELRS) / 4-in-1.
2. **Pełny dupleks telemetrii (TX powrotny do PC):**
   Pakiety telemetrii wysyłane z pojazdu RC (napięcie baterii, prąd, zużycie mAh, Link Statistics, RSSI, jakość łącza LQ, współrzędne GPS, prędkość oraz przeciążenia z IMU) są natychmiast lustrzanie przesyłane kablem USB-C z powrotem do komputera!
3. **Zero dodatkowego sprzętu na biurku (Zero Dongles):**
   Nie potrzebujesz zewnętrznych modułów Nomad, przejściówek FTDI, mostków Arduino/RP2350 ani kabli audio DSC Jack 3.5mm. Pojedynczy przewód USB-C łączy stację symulatora bezpośrednio z aparaturą!
4. **Sprzętowy Force Feedback (FFB) i Wskaźniki Kokpitu:**
   Telemetria wpadająca do RCSIM-GCS w czasie rzeczywistym napędza wskaźniki na ekranie (prędkościomierz, stan zasilania, jakość sygnału) oraz fizyczne wibracje i utratę przyczepności na silniku kierownicy Force Feedback.

---

## 📦 Zawartość Katalogu i Pliki

| Plik | Opis | Rozmiar |
|---|---|---|
| [`EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin) | Wsad Full-Duplex CRSF VCP dla **RadioMaster MT12** (Aparatura pistoletowa / kołowa) | ~528 KB |
| [`EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin) | Wsad Full-Duplex CRSF VCP dla **RadioMaster Pocket** (Aparatura drążkowa) | ~499 KB |
| [`crsf_vcp_duplex_test.py`](./crsf_vcp_duplex_test.py) | Skrypt testowy weryfikacji dwukierunkowej (nadawanie 100 Hz + dekodowanie telemetrii) | 7.4 KB |
| [`crsf_vcp_test.py`](./crsf_vcp_test.py) | Prosty skrypt testowy tylko dla kanałów (simplex) | 6.8 KB |
| [`legacy_v1_simplex/`](./legacy_v1_simplex/) | Wsady bazowe PR #7630 (tylko sterowanie, brak odbicia telemetrii) | — |
| [`README.md`](./README.md) | Dokumentacja w języku angielskim (English Documentation) | — |

---

## 🛠️ Krok 1: Wgranie Firmware do Aparatury

Najprostszą i w 100% bezpieczną metodą instalacji jest użycie wbudowanego Bootloadera EdgeTX:

1. **Dostęp do karty MicroSD:**
   - Włącz aparaturę, podłącz ją do PC kablem USB-C i na ekranie wybierz **USB Storage (SD)**.
   - Alternatywnie wyjmij kartę MicroSD z radia i włóż ją do czytnika w komputerze.
2. **Skopiowanie pliku:**
   - Wejdź do katalogu `FIRMWARE/` na karcie MicroSD.
   - Skopiuj odpowiedni plik wsadu:
     - Dla **MT12**: plik `EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin`
     - Dla **Pocket**: plik `EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin`
3. **Uruchomienie Bootloadera:**
   - Bezpiecznie odłącz radio / wysuń kartę SD.
   - Wyłącz aparaturę.
   - Ściśnij oba poziome przyciski trymerów do środka (w stronę przycisku Power) i włącz radio przyciskiem zasilania.
4. **Flashowanie:**
   - Wybierz opcję **Write Firmware**.
   - Wskaż skopiowany plik `.bin` i zatwierdź flashowanie przytrzymując rolkę/Enter.
   - Po osiągnięciu 100% wybierz **Exit**, aby uruchomić aparaturę.

---

## ⚙️ Krok 2: Konfiguracja Menu EdgeTX

Skonfiguruj model w aparaturze, aby odbierał sygnał trenera z wirtualnego portu USB:

### 1. Ustawienia Systemowe (`SYS`)
*   **Konfiguracja Radia (`SYS` -> `Radio Setup`):**
    - Ustaw `USB Mode` na **VCP** (lub **Ask**, i przy podłączaniu kabla wybieraj opcję *VCP / Serial*).
*   **Konfiguracja Sprzętowa (`SYS` -> `Hardware`):**
    - Znajdź pozycję `USB-VCP` (lub `VCP`) i zmień jej funkcję na **CRSF Trainer**.

### 2. Ustawienia Modelu (`MDL`)
*   **Tryb Trenera (`MDL` -> `Model Setup`):**
    - W sekcji **Trainer** ustaw:
      - `Mode`: **CRSF**
      - `Channels`: **CH1 - CH16**
*   **Przypisanie Funkcji Trenera (`MDL` -> `Special Functions`):**
    - Dodaj nową funkcję specjalną:
      - `Switch`: `ON` (lub wybierz fizyczny przełącznik, np. `SA` lub `SB`, którym chcesz aktywować kontrolę z PC)
      - `Action`: **Trainer**
      - `Value`: **Sticks** (lub **Axis**)
      - `Enable`: Włączona (`ON`)

---

## 🧪 Krok 3: Weryfikacja i Test Diagnostyczny

Przed uruchomieniem aplikacji RCSIM-GCS zaleca się przetestowanie transmisji dedykowanym skryptem:

1. **Włącz pojazd RC** (upewnij się, że odbiornik sparował się z radiem i nadaje telemetrię).
2. Podłącz aparaturę do PC kablem USB-C.
3. W konsoli PowerShell / CMD uruchom:
   ```bash
   python crsf_vcp_duplex_test.py
   ```
   *(Możesz podać port ręcznie: `python crsf_vcp_duplex_test.py --port COM5`)*
4. **Oczekiwany wynik w konsoli:**
   ```text
   ======================================================================
     CRSF USB-VCP Full-Duplex Test
   ======================================================================
   Otwarto port COM5 (115200 bps).
   [TX] Pętla 100 Hz uruchomiona.
   [TX] Wysłano 100 ramek sterujących (100 Hz)...
      <-- TELEMETRIA: [LINK STATS] RSSI: -46 dBm | LQ: 100% | SNR: 11 dB | RF Mode: 4
      <-- TELEMETRIA: [BATTERY] 8.24V | 1.35A | 412 mAh | 82%
      <-- TELEMETRIA: [GPS] 52.2297°N, 21.0122°E | Prędkość: 18.4 km/h | Satelity: 14
   ```
5. Na ekranie aparatury w menu **Channel Monitor** kanały 1–4 płynnie zmieniają wartości zgodnie z ramkami testowymi generowanymi przez skrypt!

---

## 🏎️ Krok 4: Integracja z RCSIM-GCS

1. Uruchom aplikację **RCSIM-GCS** na komputerze.
2. W panelu połączeń (Kokpit):
   - Wybierz wirtualny port COM aparatury (np. `COM5` - STMicroelectronics Virtual COM Port).
   - Prędkość: `115200 bps`.
   - Protokół: `CRSF Direct (USB-VCP)`.
3. W zakładce konfiguracji aparatury przypisz kierownicę, pedały oraz przyciski do wybranych kanałów CRSF.
4. Model jest gotowy do jazdy z pełnym wsparciem telemetrii i sprzętowego Force Feedbacku!

---

## 📜 Informacje Techniczne i Licencja

- **Baza EdgeTX:** Gałąź rozwojowa EdgeTX 2.10, Pull Request **#7630** (`feat/crsf-trainer-over-usb-vcp`).
- **Modyfikacja Telemetrii:** Integracja procedury `telemetrySetMirrorCb` przekierowującej odebrane pakiety telemetrii CRSF do bufora nadawczego USB-VCP.
- **Licencja:** GNU General Public License v3.0 (GPLv3).
- **Repozytorium Źródłowe EdgeTX:** [EdgeTX/edgetx](https://github.com/EdgeTX/edgetx).
