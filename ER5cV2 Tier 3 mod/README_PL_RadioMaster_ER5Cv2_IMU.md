# 🚀 RadioMaster ER5C V2 ExpressLRS (2.4GHz) – Mod z telemetrią IMU (MPU9250 / MPU6050 / MPU6500)

Rozszerzona wersja oprogramowania **ExpressLRS v4.1.0** przeznaczona dla odbiornika **RadioMaster ER5C V2 (ESP8285)**.
Oprogramowanie to dodaje pełną obsługę czujnika inercyjnego **IMU MPU9250 / MPU6050 / MPU6500 / MPU9255** przez magistralę I2C i przesyła dane telemetryczne **w czasie rzeczywistym (20Hz)** do aparatury (np. RadioMaster Nomad) i komputera PC (RCSIM GCS).

---

## 📌 Główne Funkcje Modułu IMU

- **Surowe pomiary inercyjne:** Akcelerometr ($\pm 2g$), Żyroskop ($\pm 250dps$), Magnetometr ($\mu T$).
- **Nowy typ ramki CRSF:** Ramka telemetrii `0x86` (`CRSF_FRAMETYPE_CUSTOM_IMU`), rozszerzająca standardowy protokół CRSF v3.
- **Transmisja radiowa 20Hz:** Dane przesyłane bezpośrednio z odbiornika do nadajnika i komputera.
- **Bezpieczny Heartbeat:** Jeśli czujnik jest rozłączony, odbiornik nadaje ramkę sprawdzającą z domyślnym wektorem grawitacji $1.0g$ ($9.81 m/s^2$).
- **Dynamiczna konfiguracja I2C:** Wybór dowolnych pinów magistrali I2C (SDA / SCL) z poziomu interfejsu WebUI odbiornika.

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

> 💡 **Uwaga:** Upewnij się, że zworka/pin `AD0` na czujniku MPU9250 jest zwarta do GND (adres domyślny `0x68`).

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
3. Wybierz plik: `ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin.gz`.
4. Kliknij **Update** i odczekaj na zakończenie procesu.

### Metoda B: Przez kabel FTDI USB-UART
1. Zewrzyj przycisk **BOOT** na spodzie odbiornika ER5C V2 i podłącz adapter FTDI do USB komputera.
2. Uruchom narzędzie `esptool.py` (zamieniając `COMX` na właściwy port COM):
```powershell
esptool.py --port COMX --baud 115200 --chip esp8266 write_flash -fm dout 0x00000 ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin
```

---

## 💻 Integracja w Pythonie / RCSIM (Dekoder CRSF 0x86)

W aplikacji odbiorczej po stronie komputera (np. w pliku `crsf_transceiver.py`), dodaj poniższą sekcję dekodowania ramki `0x86`:

```python
import struct

# W pętli dekodowania telemetrii CRSF:
elif frame_type == 0x86 and len(payload) >= 18:  # CRSF_FRAMETYPE_CUSTOM_IMU
    ax, ay, az, gx, gy, gz, mx, my, mz = struct.unpack(">hhhhhhhhh", payload[0:18])
    
    # Przeliczenie surowych LSB na jednostki fizyczne:
    ax_g, ay_g, az_g = ax / 16384.0, ay / 16384.0, az / 16384.0  # g (+-2g)
    gx_d, gy_d, gz_d = gx / 131.0, gy / 131.0, gz / 131.0        # deg/s (+-250dps)
    mx_u, my_u, mz_u = mx * 0.15, my * 0.15, mz * 0.15           # uT
    
    data["imu"] = {
        "ax": ax_g, "ay": ay_g, "az": az_g,
        "gx": gx_d, "gy": gy_d, "gz": gz_d,
        "mx": mx_u, "my": my_u, "mz": mz_u,
        "accel_x": ax_g, "accel_y": ay_g, "accel_z": az_g,
        "gyro_x": gx_d, "gyro_y": gy_d, "gyro_z": gz_d,
        "mag_x": mx_u, "my": my_u, "mz": mz_u,
        "raw": {
            "ax": ax, "ay": ay, "az": az,
            "gx": gx, "gy": gy, "gz": gz,
            "mx": mx, "my": my, "mz": mz,
        }
    }
```

---

## 🛠️ Zbiór Plików Wydania (Release)

W katalogu `release/` znajdują się gotowe pliki:
1. `ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin.gz` – Gotowy wsad skompresowany (dla WebUI).
2. `ELRS_V4.1_RadioMaster_ER5Cv2_MPU9250.bin` – Surowy plik binarny (dla FTDI `esptool.py`).
3. `README_PL_RadioMaster_ER5Cv2_IMU.md` – Niniejszy dokument instalacyjny.
