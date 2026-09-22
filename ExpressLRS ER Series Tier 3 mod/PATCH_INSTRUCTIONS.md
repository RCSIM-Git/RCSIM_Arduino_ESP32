# Wpięcie custom telemetrii MPU9250 do ExpressLRS (target: RadioMaster ER5A/C V2, esp8285-rx)

Zweryfikowane na żywym repo `github.com/ExpressLRS/ExpressLRS` (branch master).

## 1. Skopiuj nowe pliki

Skopiuj cały folder `IMU/` (4 pliki: `mpu9250.h`, `mpu9250.cpp`, `devImu.h`, `devImu.cpp`)
do:

    ExpressLRS/src/lib/IMU/

(dokładnie obok istniejącego `ExpressLRS/src/lib/Baro/`, ten sam poziom drzewa)

## 2. Dodaj nowy typ ramki CRSF

Plik: `src/include/crsf_protocol.h`

Znajdź enum `crsf_frame_type_e` (zaczyna się ok. linii 56) i dopisz nową wartość
tuż przed zamknięciem enuma, żeby nie kolidować z niczym istniejącym
(wszystko do `0x80` jest zajęte, `0x81`-`0xFF` wolne):

```c
    CRSF_FRAMETYPE_ARDUPILOT_RESP = 0x80,
    CRSF_FRAMETYPE_CUSTOM_IMU = 0x86,     // <-- DODAJ TĘ LINIĘ
} crsf_frame_type_e;
```

Dalej w tym samym pliku, obok innych structów payloadu (np. koło
`crsf_sensor_attitude_t`, ok. linii 363), dodaj:

```c
// CRSF_FRAMETYPE_CUSTOM_IMU
typedef struct crsf_sensor_imu_s
{
    int16_t accel_x, accel_y, accel_z; // raw MPU9250 accel, +-2g full scale
    int16_t gyro_x, gyro_y, gyro_z;    // raw MPU9250 gyro, +-250dps full scale
    int16_t mag_x, mag_y, mag_z;       // raw AK8963 mag, 0 jeśli magnetometr nie gotowy
} PACKED crsf_sensor_imu_t;
```

## 3. Zarejestruj urządzenie w RX

Plik: `src/src/rx_main.cpp`

Na górze pliku dodaj include obok innych (szukaj sekcji z `#include "devGsensor.h"`
lub podobnych, ok. początku pliku):

```cpp
#include "devImu.h"
```

W tablicy `ui_devices[]` (linia ok. 76-95) dopisz wpis analogicznie do `Baro_device`:

```cpp
device_affinity_t ui_devices[] = {
  {&Serial0_device, 1},
#if defined(PLATFORM_ESP32)
  {&Serial1_device, 1},
  {&SerialUpdate_device, 1},
#endif
  {&LED_device, 0},
  {&RXLUA_device, 0},
  {&RGB_device, 0},
  {&WIFI_device, 0},
  {&Button_device, 0},
  {&AnalogVbat_device, 0},
  {&ServoOut_device, 1},
  {&Baro_device, 0},
  {&Imu_device, 0},       // <-- DODAJ TĘ LINIĘ (kolejność po Baro nie ma znaczenia)
#if defined(PLATFORM_ESP32) && !defined(PLATFORM_ESP32_C3)
  {&VTxSPI_device, 0},
  {&MSPVTx_device, 0},
  {&Thermal_device, 0},
#endif
};
```

## 4. Dodaj nowe pliki do builda

ExpressLRS używa PlatformIO z automatycznym wykrywaniem plików w `src/lib/*`
(tak jak `Baro/` jest wykrywany bez ręcznej rejestracji w `platformio.ini`),
więc krok 1 wystarczy -- nie trzeba nic dopisywać w `.ini`.

## 5. Build

```bash
cd ExpressLRS
pio run -e RadioMaster_ER5A_C_V2_2400_RX_via_WIFI
```

(dokładna nazwa targetu do potwierdzenia w `pio run -l` -- targety RX esp8285
generowane są z `radiomaster/rx_2400/er5-v2` w repo `ExpressLRS/targets` +
sufiksem metody flashowania; jeśli nazwa się nie zgodzi, `pio run -l | grep -i er5`
pokaże dokładną etykietę)

## 6. Flash

Jedyna metoda dla tego RX: tryb WiFi (RX bez bindowania przez ~60s wchodzi w AP
"ExpressLRS RX", hasło "expresslrs") -> wgraj wygenerowany `.bin` przez WebUI.

## 7. Konfiguracja pinów I2C

To już umiesz -- w WebUI (hardware.html) ustaw dwa piny jako `I2C SCL` / `I2C SDA`.
To właśnie ustawia `i2c_enabled = true` i woła `Wire.begin(sda, scl)` w
`rxtx_common.cpp`, z czego korzysta `devImu.cpp`.

## Uwaga o częstotliwości

`IMU_PUBLISH_INTERVAL_MS` w `devImu.cpp` jest ustawione na 50ms (20Hz).
Realny throughput ograniczony jest przez skonfigurowany telemetry ratio ELRS
(1:2 do 1:128, zależnie od packet rate) -- przy niskim packet rate/wysokim
ratio i tak nie osiągniesz 20Hz na przewodzie powietrznym, ramki będą się
kolejkować i coalescować. Jeśli zobaczysz opóźnienia, podnieś packet rate
na łączu albo zwiększ interwał w kodzie.
