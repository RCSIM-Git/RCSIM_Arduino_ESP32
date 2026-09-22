# 🎮 EdgeTX — CRSF Trainer przez USB-VCP + Pełny Dupleks Telemetrii

> [!CAUTION]
> ### ⚠️ WYDANIE EKSPERYMENTALNE — UŻYWASZ NA WŁASNĄ ODPOWIEDZIALNOŚĆ
> **Ważne ostrzeżenie:**
> - Niniejsze kompilacje firmware powstały na bazie eksperymentalnego brancha EdgeTX PR **#7630** (`feat/crsf-trainer-over-usb-vcp`, commit `e5784ee5`) z dodanym autorskim patchem `telemetrySetMirrorCb` umożliwiającym zwrotne przesyłanie telemetrii przez USB.
> - **Oprogramowanie dla nowo dodanych modeli (Boxer, TX16S, TX12, TX12 MKII, Zorro) NIE BYŁO testowane na fizycznym sprzęcie ani w warunkach polowych.**
> - Przed użyciem z modelem koniecznie przetestuj działanie na biurku (bench test), obserwując zachowanie kanałów w Channel Monitor.
> - Zawsze wykonaj kopię zapasową karty MicroSD oraz obecnego firmware radia przed wgraniem nowego wsadu! Zdejmij śmigła z drona lub odłącz zasilanie silnika w aucie RC.

---

## 🚀 Przegląd i Kluczowe Korzyści

Oprogramowanie zamienia aparaturę RadioMaster w **dwukierunkowy transceiver CRSF o zerowej latencji**, wykorzystujący pojedynczy fabryczny kabel USB-C:

1. **Bezpośrednie sterowanie z PC (RX 100–250 Hz):**
   Komputer (aplikacja **RCSIM-GCS**) przesyła standardowe ramki CRSF (kanały 1–16) bezpośrednio po wirtualnym porcie szeregowym USB-C (VCP) do miksera EdgeTX i modułu nadawczego.
2. **Pełny dupleks telemetrii (TX powrotny do PC):**
   Pakiety telemetrii wysyłane z pojazdu RC (napięcie baterii, prąd, zużycie mAh, Link Statistics, RSSI, jakość łącza LQ, współrzędne GPS, prędkość oraz przeciążenia z IMU) są natychmiast lustrzanie przesyłane kablem USB-C z powrotem do komputera!
3. **Zero dodatkowego sprzętu na biurku (Zero Dongles):**
   Nie potrzebujesz zewnętrznych modułów Nomad, przejściówek FTDI, mostków Arduino/RP2350 ani kabli audio DSC Jack 3.5mm. Pojedynczy przewód USB-C łączy stację symulatora bezpośrednio z aparaturą!
4. **Sprzętowy Force Feedback (FFB) i Wskaźniki Kokpitu:**
   Telemetria wpadająca do RCSIM-GCS w czasie rzeczywistym napędza wskaźniki na ekranie (prędkościomierz, stan zasilania, jakość sygnału) oraz fizyczne wibracje i utratę przyczepności na silniku kierownicy Force Feedback.

---

## 📦 Obsługiwane Radia i Pliki Binarne

| Plik | Model Aparatury | Status Testów | Uwagi |
|---|---|---|---|
| [`EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_MT12_CRSF_VCP_FullDuplex.bin) | **RadioMaster MT12** | Przetestowane (OK) | Aparatura pistoletowa / kołowa |
| [`EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Pocket_CRSF_VCP_FullDuplex.bin) | **RadioMaster Pocket** | Przetestowane (OK) | Zoptymalizowane pod 512KB Flash |
| [`EdgeTX_v2.10_Boxer_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Boxer_CRSF_VCP_FullDuplex.bin) | **RadioMaster Boxer** | ⚠️ Eksperymentalne (Nietestowane) | Pełny Flash (F407xG) |
| [`EdgeTX_v2.10_TX16S_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_TX16S_CRSF_VCP_FullDuplex.bin) | **RadioMaster TX16S / TX16S MKII** | ⚠️ Eksperymentalne (Nietestowane) | Ekran kolorowy (2MB Flash) |
| [`EdgeTX_v2.10_TX12MK2_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_TX12MK2_CRSF_VCP_FullDuplex.bin) | **RadioMaster TX12 MKII** | ⚠️ Eksperymentalne (Nietestowane) | Zoptymalizowane pod 512KB Flash |
| [`EdgeTX_v2.10_TX12_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_TX12_CRSF_VCP_FullDuplex.bin) | **RadioMaster TX12 (V1)** | ⚠️ Eksperymentalne (Nietestowane) | Zoptymalizowane pod 512KB Flash |
| [`EdgeTX_v2.10_Zorro_CRSF_VCP_FullDuplex.bin`](./EdgeTX_v2.10_Zorro_CRSF_VCP_FullDuplex.bin) | **RadioMaster Zorro** | ⚠️ Eksperymentalne (Nietestowane) | Zoptymalizowane pod 512KB Flash |

### Dodatkowe Narzędzia
- [`crsf_vcp_duplex_test.py`](./crsf_vcp_duplex_test.py) — Skrypt testowy weryfikacji dwukierunkowej (nadawanie 100 Hz + dekodowanie telemetrii).
- [`crsf_vcp_test.py`](./crsf_vcp_test.py) — Prosty skrypt testowy tylko dla kanałów (simplex).
- [`legacy_v1_simplex/`](./legacy_v1_simplex/) — Wsady bazowe PR #7630 (tylko sterowanie, brak odbicia telemetrii).
- [`README.md`](./README.md) — Dokumentacja w języku angielskim (English Documentation).

---

## 🛠️ Krok 1: Wgranie Firmware do Aparatury

Najprostszą i w 100% bezpieczną metodą instalacji jest użycie wbudowanego Bootloadera EdgeTX:

1. **Dostęp do karty MicroSD:**
   - Włącz aparaturę, podłącz ją do PC kablem USB-C i na ekranie wybierz **USB Storage (SD)**.
   - Alternatywnie wyjmij kartę MicroSD z radia i włóż ją do czytnika w komputerze.
2. **Skopiowanie pliku:**
   - Wejdź do katalogu `FIRMWARE/` na karcie MicroSD.
   - Skopiuj odpowiedni plik wsadu pasujący do Twojego radia do katalogu `FIRMWARE/`.
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

Skonfiguruj model w aparaturze, aby przyjmował sygnał trenera z wirtualnego portu USB:

### 1. Ustawienia Systemowe (`SYS`)
*   **Radio Setup (`SYS` -> `Radio Setup`):**
    - Ustaw `USB Mode` na **VCP** (lub **Ask**, i wybieraj *VCP / Serial* po wetknięciu kabla).
*   **Hardware Configuration (`SYS` -> `Hardware`):**
    - Znajdź pozycję `USB-VCP` (lub `VCP`) i zmień jej funkcję na **CRSF Trainer**.

### 2. Ustawienia Modelu (`MDL`)
*   **Tryb Trenera (`MDL` -> `Model Setup`):**
    - Zjedź na dół do sekcji **Trainer**:
      - `Mode`: **CRSF**
      - `Channels`: **CH1 - CH16**
*   **Aktywacja Funkcji Trenera (`MDL` -> `Special Functions`):**
    - Dodaj nową funkcję specjalną:
      - `Switch`: `ON` (lub przypisz fizyczny przełącznik np. `SA` / `SB` do aktywacji)
      - `Action`: **Trainer**
      - `Value`: **Sticks** (lub **Axis**)
      - `Enable`: Zaznaczone (`ON`)

---

## 🧪 Krok 3: Weryfikacja i Diagnostyka

Przed uruchomieniem RCSIM-GCS możesz sprawdzić dwukierunkową komunikację:

1. **Włącz pojazd RC** (upewnij się, że odbiornik połączył się z radiem i nadaje telemetrię).
2. Podłącz radio kablem USB-C do PC.
3. W terminalu uruchom:
   ```bash
   python crsf_vcp_duplex_test.py
   ```
4. **Oczekiwany wynik w terminalu:**
   ```text
   ======================================================================
     CRSF USB-VCP Full-Duplex Test
   ======================================================================
   Otwarto port COM5 (115200 bps).
   [TX] Pętla 100 Hz uruchomiona.
   [TX] Wysłano 100 ramek sterujących (100 Hz)...
      <-- TELEMETRIA: [LINK STATS] RSSI: -46 dBm | LQ: 100% | SNR: 11 dB | RF Mode: 4
      <-- TELEMETRIA: [BATTERY] 8.24V | 1.35A | 412 mAh | 82%
      <-- TELEMETRIA: [GPS] 52.2297°N, 21.0122°E | Speed: 18.4 km/h | Sats: 14
   ```
5. W menu **Channel Monitor** w aparaturze obserwuj płynny ruch kanałów generowanych z PC!

---

## 🏎️ Krok 4: Integracja z RCSIM-GCS

1. Uruchom **RCSIM-GCS** na komputerze.
2. W panelu połączenia:
   - Wybierz wirtualny port COM aparatury (np. `COM5` - STMicroelectronics Virtual COM Port).
   - Baudrate: `115200 bps`.
   - Protokół: `CRSF Direct (USB-VCP)`.
3. Przypisz osie kierownicy i pedałów w zakładce konfiguracji wejść.
4. Telemetria z pojazdu na żywo napędza wskaźniki oraz efekty Force Feedback kierownicy!

---

## 📜 Informacje Techniczne i Licencja

- **Baza EdgeTX:** Gałąź EdgeTX 2.10, PR **#7630** (`feat/crsf-trainer-over-usb-vcp`).
- **Modyfikacja Telemetrii:** Integracja `telemetrySetMirrorCb` przekierowująca strumień telemetrii do bufora VCP TX.
- **Licencja:** GNU General Public License v3.0 (GPLv3).
