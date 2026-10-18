# 🛰️ RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Custom Firmware z telemetrią GPS + IMU (All-In-One)

*Dostępne języki: [Polski](README_PL_RadioMaster_ER5Cv2_GPS.md) | [English](README_EN_RadioMaster_ER5Cv2_GPS.md)*

---

Rozszerzona wersja oprogramowania **ExpressLRS v4.1.0** przeznaczona dla odbiornika **RadioMaster ER5C V2 (ESP8285)**.
Oprogramowanie to dodaje natywną obsługę modułów **GPS (UART NMEA / UBX)** oraz czujników inercyjnych **IMU (MPU9250 / MPU6050)**, przesyłając pełną telemetrię pojazdu w czasie rzeczywistym bezpośrednio do stacji naziemnej **RCSIM GCS** oraz stacji mobilnej **RCSIM MCS**.

---

## 📌 Główne Funkcje Nowego Firmware

1. **GPS przez UART:** NMEA (GGA/RMC/VTG) i dane nawigacyjne u-blox UBX. Skanowane prędkości: **115200, 9600, 38400, 57600, 230400, 19200 bps**. Zgodność zależy od faktycznego protokołu wyjściowego modułu, nie tylko jego marki.
2. **Wzbogacony parser NMEA (RMC + GGA + VTG):**
   - Obsługa pozycji, wysokości i satelitów z `$xxGGA`.
   - Pełne parsowanie prędkości (*groundspeed*) i kursu (*track heading*) bezpośrednio ze standardowej ramki `$xxRMC` (niezbędne dla tańszych modułów, które fabrycznie nie wysyłają ramki `$xxVTG`).
3. **Prawdziwy tryb All-In-One (GPS + IMU + PWM):**
   - **CH1 (GPIO0):** Pełne wyjście PWM dla serwa skrętu.
   - **CH2 (GPIO1):** Niezależne wyjście PWM dla regulatora ESC (silnik). Specjalna łata trybu `SERIAL_RX_ONLY` zapobiega blokowaniu pinu TX przez UART, pozostawiając go w 100% dla generatora PWM.
   - **CH3 (GPIO3):** Port wejściowy UART0 RX dla przewodu TX modułu GPS.
   - **CH4 (GPIO9):** I2C SDA dla czujnika inercyjnego MPU9250 / MPU6050 (lub zapasowy PWM).
   - **CH5 (GPIO10):** I2C SCL dla czujnika inercyjnego MPU9250 / MPU6050 (lub zapasowy PWM).
4. **Standardowe i rozszerzone ramki CRSF:**
   - **`0x02` (`CRSF_FRAMETYPE_GPS`):** Szerokość i długość geograficzna ($10^{-7\circ}$), prędkość ($0.1$ km/h), kurs ($0.01^\circ$), wysokość (offset 1000m), liczba satelitów.
   - **`0x03` (`CRSF_FRAMETYPE_GPS_TIME`):** Dokładny czas UTC, data oraz milisekundy.
   - **`0x13`:** Czas próbki, prędkości kątowe, przyspieszenie i temperatura; bez magnetometru. Osobna ramka `0x1E` przesyła oszacowaną orientację.

---

## 🔌 Schemat Podłączenia Sprzętowego (Pinout)

Odbiornik **RadioMaster ER5C V2** posiada 5 standardowych złączy serw modelarskich (3-pinowe: Sygnał, +, -).

### Tabela połączeń dla kompletnego pojazdu RC (Tier 3):

| Gniazdo ER5C V2 | Podłączane urządzenie | Pin modułu | Oznaczenie w WebUI | Rola |
| :--- | :--- | :--- | :--- | :--- |
| **CH1** | Serwo skrętu | Sygnał (żółty/biały) | **PWM (50Hz - 333Hz)** | Sterowanie osią skrętu |
| **CH2** | Regulator ESC (Gaz) | Sygnał (biały) | **PWM (50Hz - 400Hz)** | Sterowanie silnikiem napędowym |
| **CH3** | Moduł GPS (np. BN-220) | **TXD** modułu GPS | **Serial RX** | Odbiór ramek NMEA z GPS |
| **CH4** | IMU (MPU9250) | **SDA** | **I2C SDA** | Linia danych żyroskopu / akcelerometru |
| **CH5** | IMU (MPU9250) | **SCL** | **I2C SCL** | Linia zegarowa szyny I2C |
| **Piny `+` i `-`** | Szyna zasilania | VCC / GND | Zasilanie 5V z BEC ESC | Wspólna masa i zasilanie |

> 💡 **Ważne wskazówki podłączenia GPS:**
> - Z modułu GPS podłączamy do pinu sygnałowego **CH3** wyłącznie przewód **TX** modułu (zielony/żółty). Pin RX modułu GPS może pozostać niepodłączony.
> - Zasilanie modułu GPS (VCC i GND) wepnij do dowolnego wolnego pinu `+` oraz `-` listwy serw (odbiornik ER5C V2 ma wspólną szynę zasilania 5V BEC).

---

## ⚙️ Instrukcja Konfiguracji w WebUI Odbiornika

1. Włącz odbiornik ER5C V2 i odczekaj ok. 60 sekund bez łączenia z aparaturą, aż dioda LED zacznie szybko migać (tryb Wi-Fi Access Point).
2. Połącz się telefonem lub laptopem z siecią Wi-Fi odbiornika:
   - **SSID:** `ExpressLRS RX`
   - **Hasło:** `expresslrs`
3. Otwórz przeglądarkę pod adresem: **`http://10.0.0.1`** (lub `http://elrs_rx.local`).
4. Przejdź do zakładki **Connections / PWM Pin Functions**:
   - Dla **Output 1**: Ustaw **`50Hz PWM`** (lub żądaną częstotliwość serwa skrętu).
   - Dla **Output 2**: Ustaw **`50Hz PWM`** (lub żądaną częstotliwość ESC).
   - Dla **Output 3**: Ustaw opcję **`Serial RX`**.
   - *(Opcjonalnie)* Dla **Output 4**: Ustaw **`I2C SDA`** (jeśli używasz MPU9250).
   - *(Opcjonalnie)* Dla **Output 5**: Ustaw **`I2C SCL`** (jeśli używasz MPU9250).
5. Przejdź do sekcji **Serial/UART Options**:
   - Dla **Serial Protocol**: Wybierz **`GPS`**.
6. Kliknij **SAVE** pod tabelami i zrestartuj odbiornik.

---

## 📥 Instrukcja Wgrywania Oprogramowania

### Metoda A: Aktualizacja bezprzewodowa przez Wi-Fi WebUI (Rekomendowana)
1. Połącz się z WebUI odbiornika (`http://10.0.0.1/#update`).
2. W sekcji **Firmware Update** kliknij **Wybierz plik** i wskaż odpowiedni pakiet:
   - **`ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz`** (Zalecany dla RCSIM – jednoczesna obsługa GPS i IMU AHRS)
   - **`ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin.gz`** (Dedykowany tylko pod GPS – czysty oficjalny PR #3798 bez modułu IMU)
3. Kliknij **Update** i odczekaj ok. 15-30 sekund na ukończenie flashowania i restart urządzenia.

### Metoda B: Przez kabel FTDI USB-UART (esptool.py)
1. Wciśnij i przytrzymaj przycisk **BOOT** na spodzie ER5C V2 podczas podłączania programatora FTDI do portu USB komputera.
2. Wpisz w terminalu (zamieniając `COMX` na właściwy port):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin
```
*(lub `ELRS_V4.1_RadioMaster_ER5Cv2_GPS.bin` dla wariantu tylko z GPS)*

---

## 💻 Integracja w Pythonie / RCSIM (Dekoder CRSF 0x02 i 0x03)

W systemie RCSIM telemetria GPS jest natywnie dekodowana w `pc_app/core/comm/crsf_transceiver.py`:

```python
import struct

# W pętli asynchronicznego odbiornika telemetrii CRSF:
if frame_type == 0x02 and len(payload) >= 15:  # CRSF_FRAMETYPE_GPS
    lat, lon, speed, heading, alt = struct.unpack(">iiHHH", payload[0:14])
    sats = payload[14]
    data["gps"] = {
        "lat": lat / 1e7,
        "lon": lon / 1e7,
        "speed": speed / 10.0,       # km/h
        "heading": heading / 100.0,  # deg
        "altitude": alt - 1000,      # m
        "satellites": sats,
    }

elif frame_type == 0x03 and len(payload) >= 9:  # CRSF_FRAMETYPE_GPS_TIME
    year, month, day, hour, minute, second, ms = struct.unpack(">hBBBBBH", payload[0:9])
    data["gps_time"] = {
        "year": year, "month": month, "day": day,
        "hour": hour, "minute": minute, "second": second,
        "millisecond": ms
    }
```

---

## 🩺 Weryfikacja i usuwanie usterek

Wspólny firmware zawiera GPS UART i surowe IMU CRSF `0x13`. Stara nazwa paczki `MPU9250_CRSF13` nie oznaczała braku kodu GPS; trzeba jednak skonfigurować piny i Serial Protocol.

- GPS: sprawdź status GPS w WebUI odbiornika (protokół, baud, wiek próbki, fix, satelity), następnie ramkę CRSF `0x02` w RCSIM. Wykrycie prędkości UART nie oznacza fixa satelitarnego.
- IMU: sprawdź payload `0x13` długości 18 bajtów i `source: crsf_raw`. Błąd I2C nie generuje sztucznych pomiarów ani heartbeat 1 g. Zobacz [podłączenie i dekodowanie IMU](README_PL_RadioMaster_ER5Cv2_IMU.md).
- GPS jednokierunkowy: CH3 jako Serial RX, główny Serial Protocol jako GPS, CH2 pozostaje PWM. TX GPS → CH3; RX GPS niepodłączony. GPS musi już nadawać obsługiwane NMEA lub UBX; w tym trybie nie docierają do niego polecenia konfiguracji.
- Opcjonalny GPS dwukierunkowy: ustaw CH2/GPIO1 jako Serial TX i podłącz do RX GPS. Umożliwia to próby konfiguracji u-blox, ale **CH2 nie może już sterować ESC**. W razie potrzeby użyj innego odpowiedniego wyjścia sterującego.
- Zasilanie: napięcie zgodne ze specyfikacją modułu, wspólna masa i sygnały zgodne z 3,3 V; kieruj się oznaczeniami, nie kolorami przewodów.
- Brak danych UART: sprawdź TX→RX, funkcję CH3, Serial Protocol i zasilanie. Dane bez rozpoznania: sprawdź protokół modułu i obsługiwane baud. Fix wymaga odsłoniętego nieba.
- Weryfikuj GPS i IMU osobno. Hz pakietów łącza nie oznacza Hz nowych próbek IMU; pętla IMU ma 100 Hz, odbiór zależy od pasma telemetrii.

---

## 🛠️ Zbiór Plików Wydania (Release)

Pliki binarne znajdują się w niniejszym katalogu:
1. `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz` – Gotowy wsad skompresowany (dla WebUI OTA Wi-Fi).
2. `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin` – Surowy plik binarny (dla FTDI / `esptool.py`).
3. `README_PL_RadioMaster_ER5Cv2_GPS.md` – Niniejsza dokumentacja techniczna (PL).
4. `README_EN_RadioMaster_ER5Cv2_GPS.md` – Dokumentacja w języku angielskim (EN).
