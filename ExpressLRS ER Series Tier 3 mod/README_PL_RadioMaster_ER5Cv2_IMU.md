# 🚀 RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Mod GPS + IMU (MPU9250 / MPU6050 / MPU6500)

*Dostępne języki: [Polski](README_PL_RadioMaster_ER5Cv2_IMU.md) | [English](README_EN_RadioMaster_ER5Cv2_IMU.md)*

---

Rozszerzona wersja oprogramowania **ExpressLRS v4.1.0** przeznaczona dla odbiornika **RadioMaster ER5C V2 (ESP8285)**.
Oprogramowanie to dodaje pełną obsługę czujnika inercyjnego **IMU MPU9250 / MPU6050 / MPU6500 / MPU9255** przez magistralę I2C i przesyła dane telemetryczne **w czasie rzeczywistym; pętla IMU pracuje co 10 ms (100 Hz), a rzeczywista częstotliwość odbioru zależy od pasma telemetrii** do aparatury (np. RadioMaster Nomad) i komputera PC (RCSIM GCS).

---


## 📌 Główne Funkcje Modułu IMU

- **Wspólny firmware GPS + IMU:** GPS przez UART (NMEA / UBX) i IMU przez I2C działają jednocześnie.
- **Zmierzone przyspieszenie i prędkość kątowa:** Zakresy sprzętowe MPU to ±2 g i ±250 deg/s; standardowa ramka CRSF `0x13` koduje ±16 g i ±2000 deg/s.
- **Standardowe ramki CRSF:** `0x13` zawiera czas próbki, żyroskop, akcelerometr i temperaturę; `0x1E` osobno przesyła oszacowaną orientację. GPS używa `0x02`, a czas GPS `0x03`.
- **Bez sztucznych surowych próbek:** Nieudany odczyt I2C nie generuje pomiaru `0x13`. Ta ramka nie zawiera magnetometru.
- **Konfiguracja I2C:** Ustaw SDA/SCL w WebUI. Rzeczywistą częstotliwość próbek sprawdzaj w RCSIM, nie na podstawie częstotliwości pakietów łącza.

---

## 🔌 Schemat Podłączenia Sprzętowego (Pinout)

Odbiornik **RadioMaster ER5C V2** posiada 5 kanałów serw PWM. Używamy **Kanału 4 (CH4)** oraz **Kanału 5 (CH5)** do podłączenia magistrali I2C czujnika IMU.

### Tabela połączeń:

| Pin czujnika IMU (MPU9250) | Pin odbiornika RadioMaster ER5C V2 | Oznaczenie w WebUI |
| :--- | :--- | :--- |
| **VCC** | **`+`** (Linia zasilania PWM) | Zasilanie 3.3V / 5V |
| **GND** | **`-`** (Masa PWM) | Masa (GND) |
| **SDA** | **`~` (Sygnałowy Kanał 4 / CH4)** | **I2C SDA** |
| **SCL** | **`~` (Sygnałowy Kanał 5 / CH5)** | **I2C SCL** |

> 💡 **Uwaga:** Zarówno adres **`0x68`** (AD0=GND), jak i **`0x69`** (AD0=HIGH, np. na modułach SEN-MPU6050) są wykrywane i obsługiwane automatycznie w locie!

---

## 🛰️ Podłączenie GPS razem z IMU

- TX modułu GPS → **sygnał CH3 (GPIO3)**; ustaw Output 3 jako **Serial RX**, a główny **Serial Protocol** jako **GPS**.
- Pozostaw **CH2 (GPIO1)** w trybie PWM, aby zachować wyjście ESC. Przy odbiorze jednokierunkowym RX GPS pozostaje niepodłączony; polecenia autokonfiguracji nie docierają do GPS, więc moduł musi już nadawać obsługiwane NMEA lub UBX.
- CH1 pozostaje PWM kierownicy; CH4 (GPIO9) to SDA IMU, CH5 (GPIO10) to SCL IMU.
- Zapewnij wspólną masę. Dobierz napięcie zasilania do specyfikacji modułu; napięcie szyny PWM nie musi być bezpieczne dla samego MPU. Podciąganie sygnałów I2C musi być do 3,3 V.
- Zapisz ustawienia i zrestartuj odbiornik. Zweryfikuj osobno ramki GPS `0x02` i IMU `0x13`; fix GPS wymaga odbioru satelitów, ale nie jest potrzebny do pomiarów IMU.

---

## ⚙️ Instrukcja Konfiguracji w WebUI

1. Włącz odbiornik ER5C V2 i odczekaj ok. 60 sekund bez łączenia z aparaturą.
2. Połącz się z punktem Wi-Fi odbiornika:
   - **SSID:** `ExpressLRS RX`
   - **Hasło:** `expresslrs`
3. Otwórz przeglądarkę pod adresem: **`http://10.0.0.1`** (lub adres przydzielony przez domowy router).
4. Przejdź do zakładki **Connections / PWM Pin Functions**:
   - Dla **Output 4**: ustaw opcję **`I2C SDA`**
   - Dla **Output 5**: ustaw opcję **`I2C SCL`**
5. Kliknij **SAVE** pod tabelą i zrestartuj odbiornik.

---

## 📥 Instrukcja Wgrywania Oprogramowania

### Metoda A: Aktualizacja bezprzewodowa przez Wi-Fi WebUI (Rekomendowana)
1. Połącz się z WebUI odbiornika (`http://10.0.0.1` lub `http://elrs_rx.local`).
2. Przejdź do zakładki **Update** (`http://10.0.0.1/#update`).
3. Wybierz plik: `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz`.
4. Kliknij **Update** i odczekaj na zakończenie procesu.

### Metoda B: Przez kabel FTDI USB-UART
1. Zewrzyj przycisk **BOOT** na spodzie odbiornika ER5C V2 i podłącz adapter FTDI do USB komputera.
2. Uruchom narzędzie `esptool.py` (zamieniając `COMX` na właściwy port COM):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin
```

---

## 💻 RCSIM / CRSF 0x13

Aktualny RCSIM dekoduje `0x13`; nie dodawaj przestarzałego dekodera `0x86`. Sprawdź `Raw Crsf → 0x13`, payload 18 bajtów, `source: crsf_raw`, temperaturę i reagujący żyroskop. Sama ramka `0x1E` oznacza orientację, nie surowy pomiar IMU. Cały tor RX → TX → USB/serial musi przekazywać `0x13`.

Przykład dekodowania payloadu (big-endian; znak gyro X przeliczony z lewostronnego CRSF na prawostronny RCSIM):

```python
import struct

# Only after CRSF frame length and CRC validation:
if frame_type == 0x13 and len(payload) == 18:
    timestamp, gx, gy, gz, ax, ay, az, temp = struct.unpack(">I7h", payload)
    accel_g = tuple(v * 16.0 / 32767.0 for v in (ax, ay, az))
    gyro_dps = (-gx * 2000.0 / 32767.0,
                gy * 2000.0 / 32767.0, gz * 2000.0 / 32767.0)
    temperature_c = temp / 100.0
```

---

## 🛠️ Zbiór Plików Wydania (Release)

W katalogu `release/` znajdują się gotowe pliki:
1. `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin.gz` – Gotowy wsad skompresowany (dla WebUI).
2. `ELRS_V4.1_RadioMaster_ER5Cv2_RCSIM_GPS_IMU.bin` – Surowy plik binarny (dla FTDI `esptool.py`).
3. `README_PL_RadioMaster_ER5Cv2_IMU.md` – Niniejszy dokument instalacyjny (wersja PL).
4. `README_EN_RadioMaster_ER5Cv2_IMU.md` – Dokumentacja w języku angielskim (wersja EN).
