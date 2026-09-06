# 🛰️ RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Custom Firmware z telemetrią GPS + IMU (All-In-One)

*Dostępne języki: [Polski](README_PL_RadioMaster_ER5Cv2_GPS.md) | [English](README_EN_RadioMaster_ER5Cv2_GPS.md)*

---

Rozszerzona wersja oprogramowania **ExpressLRS v4.1.0** przeznaczona dla odbiornika **RadioMaster ER5C V2 (ESP8285)**.
Oprogramowanie to dodaje natywną obsługę modułów **GPS (UART NMEA / UBX)** oraz czujników inercyjnych **IMU (MPU9250 / MPU6050)**, przesyłając pełną telemetrię pojazdu w czasie rzeczywistym bezpośrednio do stacji naziemnej **RCSIM GCS** oraz stacji mobilnej **RCSIM MCS**.

---

## 📌 Główne Funkcje Nowego Firmware

1. **Uniwersalna obsługa dowolnego modułu GPS:**
   - Kompatybilność z każdym popularnym modułem GPS na rynku: **Beitian (BN-180, BN-220, BN-880)**, **Foxeer (M10, M8)**, **Matek (M8Q, M10)**, **TBS M8**, **RadioMaster**, **u-blox (NEO-6M, NEO-7M, NEO-8M, M9N, M10)** oraz **ATGM336H**.
   - **Automatyczna detekcja prędkości (Auto-Baudrate):** Odbiornik automatycznie cyklicznie wykrywa prędkość portu (**115200, 9600, 38400, 57600 bps**) i po weryfikacji sumy kontrolnej NMEA blokuje się na właściwej prędkości. Moduły działają od razu po wyjęciu z pudełka bez konieczności rekonfiguracji w u-center.
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
   - **`0x86` (`CRSF_FRAMETYPE_CUSTOM_IMU`):** Surowe przeciążenia ($ax, ay, az$), prędkości kątowe ($gx, gy, gz$) oraz pole magnetyczne ($mx, my, mz$).

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
   - Dla **Serial 1 Protocol**: Wybierz **`GPS`**.
6. Kliknij **SAVE** pod tabelami i zrestartuj odbiornik.

---

## 📥 Instrukcja Wgrywania Oprogramowania

### Metoda A: Aktualizacja bezprzewodowa przez Wi-Fi WebUI (Rekomendowana)
1. Połącz się z WebUI odbiornika (`http://10.0.0.1/#update`).
2. W sekcji **Firmware Update** kliknij **Wybierz plik** i wskaż:
   `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz`
3. Kliknij **Update** i odczekaj ok. 15-30 sekund na ukończenie flashowania i restart urządzenia.

### Metoda B: Przez kabel FTDI USB-UART (esptool.py)
1. Wciśnij i przytrzymaj przycisk **BOOT** na spodzie ER5C V2 podczas podłączania programatora FTDI do portu USB komputera.
2. Wpisz w terminalu (zamieniając `COMX` na właściwy port):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin
```

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

## 🛠️ Zbiór Plików Wydania (Release)

Pliki binarne znajdują się w niniejszym katalogu:
1. `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz` – Gotowy wsad skompresowany (dla WebUI OTA Wi-Fi).
2. `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin` – Surowy plik binarny (dla FTDI / `esptool.py`).
3. `README_PL_RadioMaster_ER5Cv2_GPS.md` – Niniejsza dokumentacja techniczna (PL).
4. `README_EN_RadioMaster_ER5Cv2_GPS.md` – Dokumentacja w języku angielskim (EN).
