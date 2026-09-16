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

## 🩺 Diagnostyka Sprzętowa IMU w Czasie Rzeczywistym (Kody Błędów I2C)

Gdy odbiornik nie może nawiązać fizycznej komunikacji z czujnikiem IMU (`!imu_initialized`), zamiast wysyłać zera, przesyła w surowej telemetrii ramki `0x86` (`Raw`) dokładne kody stanu magistrali I2C:

| Oś w RCSIM (`Raw`) | Zmienna w firmware | Wartość | Znaczenie i Diagnoza |
| :--- | :--- | :---: | :--- |
| **Acceleration X** | `diag_err68` | **`0`**<br>**`2`**<br>**`3`**<br>**`4`**<br>**`99`**<br>**`255`** | **0** = Połączenie I2C OK (odebrano ACK)<br>**2** = **Brak układu pod 0x68 (NACK on address)**<br>**3** = NACK podczas transmisji danych<br>**4** = Inny błąd magistrali (np. zwarcie)<br>**99** = Stan oczekiwania (brak próby odczytu)<br>**255** = Szyna I2C wyłączona (brak trybów SDA/SCL w WebUI) |
| **Acceleration Y** | `diag_whoami68` | **`0x71`** (113)<br>**`0x73`** (115)<br>**`0x68`** (104)<br>**`0x70`** (112)<br>**`0`** | **MPU-9250**<br>**MPU-9255**<br>**MPU-6050**<br>**MPU-6500**<br>**0 = Brak odpowiedzi rejestru WHO_AM_I** |
| **Acceleration Z** | Domyślny wektor | **`16384`** | Stała grawitacji $1.0g$ ($9.81 m/s^2$) – bezpieczny sygnał heartbeat |
| **Angular velocity X** | `diag_err69` | Kody jak w `diag_err68` | Sprawdzenie alternatywnego adresu I2C `0x69` (gdy AD0 = VCC) |
| **Angular velocity Y** | `diag_whoami69` | Identyfikatory chipu | Odczytany rejestr WHO_AM_I pod adresem `0x69` |
| **Angular velocity Z** | `imu_address` | **`104`** (`0x68`)<br>**`105`** (`0x69`) | Aktualnie sprawdzany adres I2C |

### Szybka ściągawka usuwania usterek IMU:
- **`Acceleration X = 2` oraz `Angular velocity X = 2`:** Układ nie odpowiada elektrycznie na magistrali:
  1. Zamień miejscami przewody sygnałowe **CH4 (SDA)** i **CH5 (SCL)**.
  2. Sprawdź zasilanie VCC oraz wspólną masę GND z pinem `-` odbiornika.
  3. Upewnij się, że wtyczka w gnieździe serw nie jest odwrócona do góry nogami (sygnał jest na górnym pinie).
- **`Acceleration X = 0`, ale brak danych ruchu:** Układ odpowiedział ACK pod 0x68, ale `Acceleration Y` (WHO_AM_I) nie pasuje do znanych chipów z rodziny MPU.
- **`Acceleration X = 255`:** W WebUI odbiornika na piny CH4 i CH5 nie ustawiono opcji `I2C SDA` i `I2C SCL`.

---

## 🛰️ Diagnostyka Sprzętowa GPS w Czasie Rzeczywistym (Kody Stanu UART / NMEA)

Gdy odbiornik nie zablokował jeszcze prawidłowych ramek NMEA z GPS (`validPacketsCount == 0`) lub nastąpiła utrata sygnału (> 3s), odbiornik ER5C V2 wysyła cyklicznie (co 1000 ms) specjalną ramkę diagnostyczną `0x02` (`CRSF_FRAMETYPE_GPS`) z `Latitude = 0` i `Longitude = 0`.

Dzięki temu zarówno w **RCSIM GCS**, jak i bezpośrednio na ekranie aparatury **EdgeTX / OpenTX** (sensory `Sats`, `GSpd`, `GAlt`, `Hdg`), od razu widać fizyczny stan połączenia z modułem GPS bez konieczności podłączania debuggera:

| Pole CRSF (`0x02`) | Sensor EdgeTX | Pole w RCSIM | Znaczenie i Wartość Diagnostyczna |
| :--- | :--- | :--- | :--- |
| **`Satellites`** | `Sats` | `gps["satellites"]`<br>`diagnostic["state_code"]` | **Kod stanu połączenia z GPS:**<br>• **`0` (`NO_DATA`):** Brak jakichkolwiek bajtów na pinie RX (CH3). Linia jest całkowicie głucha.<br>• **`1` (`SCANNING_BAUD`):** Bajty przychodzą, trwa skanowanie prędkości UART.<br>• **`2` (`CHECKSUM_FAIL`):** Bajty przychodzą, ale ramki nie przechodzą sumy kontrolnej NMEA XOR.<br>• **`3` (`BAUD_LOCKED`):** Prędkość UART dopasowana i zablokowana, oczekiwanie na pełne ramki.<br>• **`4` (`LOST_TIMEOUT`):** Utrata strumienia GPS w trakcie pracy (brak poprawnych ramek > 3s). |
| **`Groundspeed`** | `GSpd` | `gps["speed"]`<br>`diagnostic["baud_rate"]` | **Aktualnie testowany / zablokowany Baudrate:**<br>• **`115.2 km/h`** = **115200 bps**<br>• **`9.6 km/h`** = **9600 bps**<br>• **`38.4 km/h`** = **38400 bps**<br>• **`57.6 km/h`** = **57600 bps** |
| **`Altitude`** | `GAlt` | `gps["altitude"]`<br>`diagnostic["bytes_received"]` | **Licznik odebranych bajtów na pinie CH3** (`rawBytesCount % 10000`):<br>• **`0 m`** = Ani jeden bajt nie dotarł do odbiornika (brak sygnału).<br>• **`> 0 m`** (np. 45 m, 120 m...) = Tyle bajtów fizycznie odebrał UART. Jeśli liczba rośnie, linia fizyczna i zasilanie GPS działają! |
| **`Heading`** | `Hdg` | `gps["heading"]`<br>`diagnostic["csum_errors"]` | **Licznik błędów sumy kontrolnej NMEA** (`csumErrors / 100.0`):<br>• Pokazuje ile ramek miało niepoprawny CRC (np. z powodu złego baudrate lub szumu). |

### 🔍 Szybka ściągawka usuwania usterek GPS:
- **`Sats = 0`, `GAlt = 0` (stan `NO_DATA`):**
  1. Sprawdź, czy przewód **TX** modułu GPS jest wpięty do pinu sygnałowego **CH3** odbiornika.
  2. Sprawdź, czy moduł GPS ma zasilanie 5V (VCC) i wspólną masę (GND) z odbiornikiem.
  3. Upewnij się, że nie zamieniłeś TX z RX modułu GPS (odbiornik ER5C V2 potrzebuje sygnału **TX z GPS wpiętego do CH3**).
- **`Sats = 1` lub `2`, `GAlt > 0` rośnie (stan `SCANNING` / `CHECKSUM_FAIL`):**
  1. Sygnał fizyczny dociera do odbiornika, ale żaden pakiet nie tworzy poprawnej ramki NMEA.
  2. Moduł GPS może być fabrycznie ustawiony na binarny protokół u-blox UBX bez NMEA (włącz NMEA w programie u-center).
  3. Moduł nadaje z niestandardową prędkością (np. 4800 lub 230400 bps) poza zakresem Auto-Baud (9600-115200 bps).
- **Gdy GPS złapie prawidłowe pakiety NMEA (`validPacketsCount > 0`):**
  1. Tryb diagnostyczny natychmiast automatycznie ustępuje miejsca prawdziwej telemetrii nawigacyjnej.
  2. Odbiornik zaczyna transmitować rzeczywiste współrzędne geograficzne (`Latitude`, `Longitude`), realną wysokość n.p.m., prędkość, kurs i liczbę śledzonych satelitów (0..32+).

---

## 🛠️ Zbiór Plików Wydania (Release)

Pliki binarne znajdują się w niniejszym katalogu:
1. `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz` – Gotowy wsad skompresowany (dla WebUI OTA Wi-Fi).
2. `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin` – Surowy plik binarny (dla FTDI / `esptool.py`).
3. `README_PL_RadioMaster_ER5Cv2_GPS.md` – Niniejsza dokumentacja techniczna (PL).
4. `README_EN_RadioMaster_ER5Cv2_GPS.md` – Dokumentacja w języku angielskim (EN).
