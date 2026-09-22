# 🛰️ Modyfikacja ExpressLRS v4.1: Obsługa GPS + Ochrona ESC PWM (Target: RadioMaster ER5C V2 ESP8285)

Instrukcja krok po kroku opisująca zmiany w kodzie źródłowym **ExpressLRS v4.1.0**, umożliwiające podłączenie dowolnego modułu GPS (UART NMEA 9600 - 115200 bps) do odbiornika **RadioMaster ER5C V2** przy jednoczesnym zachowaniu wyjścia PWM dla regulatora obrotów silnika (ESC).

---

## 🎯 Cel i Rozwiązane Problemy

1. **Konflikt pinu TX (GPIO1 / CH2):**
   - W standardowym ExpressLRS wybranie protokołu `PROTOCOL_GPS` na porcie Serial0 uruchamiało UART w trybie dwukierunkowym (`SERIAL_FULL`).
   - Na odbiorniku ER5C V2 pin GPIO1 jest fizycznie wyprowadzony na gniazdo **CH2 (Gaz / ESC)**. W efekcie regulator silnika tracił sygnał PWM i silnik nie reagował.
   - **Rozwiązanie:** Wdrożenie trybu `SERIAL_RX_ONLY` w `rx_main.cpp` oraz zabezpieczenie w `RXParameters.cpp`, aby pin GPIO1 pozostał w 100% wyjściem sprzętowym PWM (`som50Hz` .. `som400Hz`).

2. **Obsługa modułów GPS o różnych prędkościach (Auto-Baudrate):**
   - Fabryczne moduły GPS (np. Beitian BN-180, BN-220, NEO-6M, ATGM336H) często nadają z prędkością 9600 bps, podczas gdy ELRS oczekiwał na sztywno 115200 bps.
   - **Rozwiązanie:** Mechanizm automatycznego próbkowania prędkości (115200 $\rightarrow$ 9600 $\rightarrow$ 38400 $\rightarrow$ 57600 bps) co 1.5 sekundy w `SerialGPS.cpp`. Po pierwszej ramce NMEA z poprawną sumą kontrolną odbiornik blokuje właściwy baudrate.

3. **Wzbogacenie parsera NMEA:**
   - Wiele budżetowych modułów nie emituje ramki `$xxVTG`.
   - **Rozwiązanie:** Bezpośrednie dekodowanie prędkości (knots $\rightarrow$ km/h) oraz kursu (*track angle*) ze standardowej ramki `$xxRMC`.

---

## 📝 Zmiany w Kodzie Źródłowym ExpressLRS

### 1. Zabezpieczenie pinu CH2 przed wymuszeniem trybu Serial
**Plik:** `src/lib/rx-crsf/RXParameters.cpp`  
**Funkcja:** `configureSerialPin()` (ok. linii 418)

```cpp
      // If the new mode is serial, the sibling is also forced to serial
      // unless GPS protocol is used and sibling is the TX pin (GPIO1)
      if (newMode == somSerial)
      {
        if (config.GetSerialProtocol() != PROTOCOL_GPS || sibling != 1)
        {
          siblingPinConfig.val.mode = somSerial;
        }
      }
```

---

### 2. Auto-Baudrate i Rozszerzenie Klasy `SerialGPS`
**Plik:** `src/src/rx-serial/SerialGPS.h`

W klasie `SerialGPS` dodaj wskaźnik do `HardwareSerial` w konstruktorze oraz zmienne stanu auto-baudrate:

```cpp
class HardwareSerial;

class SerialGPS final : public SerialIO {
public:
    explicit SerialGPS(Stream &out, Stream &in, HardwareSerial *hw = nullptr) 
        : SerialIO(&out, &in), _hwPort(hw) {}
    ~SerialGPS() override = default;

    // ...
private:
    char nmeaBuffer[83] = {0};
    uint8_t nmeaBufferIndex = 0;

    HardwareSerial *_hwPort = nullptr;
    bool hasGga = false;
    uint8_t currentBaudIndex = 0;
    bool baudLocked = false;
    uint32_t lastBaudSwitchMs = 0;
    uint32_t validPacketsCount = 0;
};
```

---

### 3. Implementacja Auto-Baudrate i Parsowania RMC
**Plik:** `src/src/rx-serial/SerialGPS.cpp`

Dopisz tabelę prędkości i logikę przełączania w `sendQueuedData()`:

```cpp
#include <HardwareSerial.h>

static const uint32_t gpsBaudRates[] = { 115200, 9600, 38400, 57600 };
static const uint8_t gpsBaudRatesCount = sizeof(gpsBaudRates) / sizeof(gpsBaudRates[0]);

void SerialGPS::sendQueuedData(uint32_t maxBytesToSend)
{
    if (!baudLocked && _hwPort != nullptr)
    {
        uint32_t now = millis();
        if (lastBaudSwitchMs == 0)
        {
            lastBaudSwitchMs = now;
        }
        else if (now - lastBaudSwitchMs >= 1500)
        {
            lastBaudSwitchMs = now;
            currentBaudIndex = (currentBaudIndex + 1) % gpsBaudRatesCount;
            uint32_t newBaud = gpsBaudRates[currentBaudIndex];
            _hwPort->flush();
            _hwPort->updateBaudRate(newBaud);
            nmeaBufferIndex = 0;
            DBGLN("GPS Auto-Baud: trying %u baud", newBaud);
        }
    }
}
```

W `isValidChecksum()` zablokuj prędkość po pierwszym poprawnym pakiecie:

```cpp
    if (!baudLocked)
    {
        baudLocked = true;
        DBGLN("GPS Baud locked at %u", gpsBaudRates[currentBaudIndex]);
    }
    validPacketsCount++;
    return true;
```

W `fieldParseRMC()` dodaj obsługę współrzędnych, prędkości i kursu:

```cpp
        case 3: // Latitude: DDMM.MMMMM
            ctx->gpsData.lat = nmeaDdmToDd(field);
            break;
        case 4: // N/S
            if (field[0] == 'S') ctx->gpsData.lat = -ctx->gpsData.lat;
            break;
        case 5: // Longitude: DDDMM.MMMMM
            ctx->gpsData.lon = nmeaDdmToDd(field);
            break;
        case 6: // E/W
            if (field[0] == 'W') ctx->gpsData.lon = -ctx->gpsData.lon;
            break;
        case 7: // Speed over ground: knots -> scaled km/h * 100
        {
            int32_t knots100 = parseDecimalToScaled(field, 100);
            ctx->gpsData.speed = (uint32_t)((knots100 * 1852LL + 500) / 1000);
            break;
        }
        case 8: // Track angle in degrees -> scaled by 100
            ctx->gpsData.heading = parseDecimalToScaled(field, 100);
            break;
```

W `processSentence()` emituj ramkę `sendTelemetryFrame()` również z RMC, jeśli moduł nie nadaje GGA:

```cpp
    else if (sentence[3] == 'R' && sentence[4] == 'M' && sentence[5] == 'C') {
        splitSentenceFields(sentence, size, &fieldParseRMC);
        sendGpsTimeTelemetryFrame();
        if (!hasGga) {
            sendTelemetryFrame();
        }
    }
```

---

### 4. Inicjalizacja `SERIAL_RX_ONLY` w RX Main
**Plik:** `src/src/rx_main.cpp`  
**Funkcja:** `setupSerial()` (ok. linii 1329)

```cpp
    SerialMode mode = (sbusSerialOutput || sumdSerialOutput) ? SERIAL_TX_ONLY : SERIAL_FULL;
    if (config.GetSerialProtocol() == PROTOCOL_GPS)
    {
        // GPS na ESP8285 potrzebuje wyłącznie wejścia RX (GPIO3 / CH3).
        // Trwale uwalniamy pin TX (GPIO1 / CH2) dla wyjścia PWM regulatora ESC!
        mode = SERIAL_RX_ONLY;
    }
    Serial.begin(serialBaud, serialConfig, mode, -1, invert);
```

Przekazanie instancji portu `HardwareSerial` do `SerialGPS`:

```cpp
    else if (config.GetSerialProtocol() == PROTOCOL_GPS)
    {
        serialIO = new SerialGPS(SERIAL_PROTOCOL_TX, SERIAL_PROTOCOL_RX, &SERIAL_PROTOCOL_RX);
    }
```

---

### 5. Zwolnienie i generowanie PWM dla CH2 (ESC) w `devServoOutput`
**Plik:** `src/lib/ServoOutput/devServoOutput.cpp`

WebUI ExpressLRS w przeglądarce automatycznie wymusza wyświetlanie i zapis pinu TX jako `Serial TX`, gdy pin RX ustawiony jest na `Serial RX`. Aby odbiornik generował pełny sygnał PWM 50Hz dla ESC na kanale CH2 niezależnie od etykiety w WebUI:

W funkcji `start()` (ok. linii 262):
```cpp
        auto mode = (eServoOutputMode)config.GetPwmChannel(ch)->val.mode;
#if defined(PLATFORM_ESP8266)
        if (config.GetSerialProtocol() == PROTOCOL_GPS && pin == 1)
        {
            // GPS na ESP8285 używa wyłącznie RX. Wymuszamy 50Hz PWM na CH2 dla ESC:
            if (mode >= somSerial)
            {
                mode = som50Hz;
            }
        }
        else
#endif
        if (mode >= somSerial)
        {
            pin = UNDEF_PIN;
        }
```

W funkcji `event()` (ok. linii 345):
```cpp
        for (int ch = 0; ch < GPIO_PIN_PWM_OUTPUTS_COUNT; ++ch)
        {
            const rx_config_pwm_t *chConfig = config.GetPwmChannel(ch);
            auto mode = (eServoOutputMode)chConfig->val.mode;
#if defined(PLATFORM_ESP8266)
            if (config.GetSerialProtocol() == PROTOCOL_GPS && GPIO_PIN_PWM_OUTPUTS[ch] == 1)
            {
                if (mode >= somSerial)
                {
                    mode = som50Hz;
                }
            }
#endif
            const auto frequency = servoOutputModeToFrequency(mode);
            if (frequency && servoPins[ch] != UNDEF_PIN)
            {
                pwmChannels[ch] = PWM.allocate(servoPins[ch], frequency);
            }
        }
```

---

### 6. Rozpięcie parowania TX/RX w WebUI i devWIFI
Gdy na pinie CH3 wybierano `Serial RX`, interfejs ExpressLRS automatycznie wymuszał na CH2 tryb `Serial TX` i wyszarzał możliwość konfiguracji kanału (`Input`).

**Plik 1:** `src/html/src/pages/connections-panel.js`
W `_pinModeChange()` uniezależniono pin TX od pinu RX, gdy aktywny jest protokół GPS (`elrsState.config['serial-protocol'] === 9`), dzięki czemu CH2 pozostaje w 100% edytowalnym wyjściem serwa:
```javascript
        if (this.pinRxIndex !== undefined && this.pinTxIndex !== undefined) {
            const isGps = elrsState.config['serial-protocol'] === 9
            // Nie wymuszaj Serial TX na pinie CH2 jeśli aktywny jest GPS
            if (index === this.pinTxIndex && pinTxModeValue === PWM_MODE_SERIAL && !isGps) {
                pinRxMode.value = PWM_MODE_SERIAL
                setDisabled(this.pinRxIndex, true)
                setDisabled(this.pinTxIndex, true)
                pinTxMode.disabled = true
            }
        }
```

**Plik 2:** `src/lib/WIFI/devWIFI.cpp`
W `GetConfiguration()` wyłączono flagę `features |= 1` (SerialTX) dla pinu GPIO1 przy aktywnym protokole GPS, a w `UpdateConfiguration()` zabezpieczono zapis przed nadpisaniem go jako `somSerial`.

---

### 7. Telemetria Diagnostyczna I2C IMU i Naprawa Zatrzasku GPOC ESP8285
**Pliki:** `src/src/rxtx_common.cpp`, `src/lib/IMU/devImu.cpp`, `src/lib/IMU/mpu9250.cpp`

1. **Usunięcie błędu zatrzasku wyjścia (stuck HIGH latch bug):**
   W bibliotece Wire na ESP8266/ESP8285 sterownik software I2C (`core_esp8266_si2c.cpp`) symuluje open-drain poprzez ustawianie `GPES` (załączenie drivera wyjścia do ściągania do masy GND). Wymaga to, aby zatrzask danych wyjściowych (`GPOC`) wynosił `0`. Poprzednia sekwencja recovery zostawiała `digitalWrite(HIGH)`, przez co driver zamiast ściągać do 0V, wystawiał stale 3.3V, wywołując trwały błąd NACK (kod 2) na każdym podłączonym sensorze. W `setupWire()` oraz przed detekcją wyzerowano rejestry `GPOC = (1 << sda) | (1 << scl)`.
2. **Automatyczne wykrywanie zamiany pinów SDA $\leftrightarrow$ SCL:**
   W `devImu.cpp`, gdy pod adresem 0x68 i 0x69 pojawia się błąd 2, odbiornik natychmiast automatycznie próbuje odwróconej pary pinów `(SCL, SDA)`. Jeśli IMU odpowie, odbiornik trwale blokuje działającą orientację przewodów bez konieczności fizycznego ich przepinania.
3. **Kody stanu magistrali I2C w ramce 0x86 (sekcja Raw w RCSIM):**
   - `Acceleration X` = `diag_err68` (0 = OK, **2 = NACK brak układu pod 0x68**, 255 = I2C wyłączone w WebUI)
   - `Acceleration Y` = `diag_whoami68` (odczytany rejestr WHO_AM_I pod 0x68)
   - `Acceleration Z` = `16384` (stała grawitacji $1.0g$ jako bezpieczny heartbeat)
   - `Angular velocity X` = `diag_err69` (kod błędu pod adresem 0x69)
   - `Angular velocity Y` = `diag_whoami69` (WHO_AM_I pod adresem 0x69)
   - `Angular velocity Z` = `imu_address` (104 = 0x68, 105 = 0x69)

---

### 8. Telemetria Diagnostyczna GPS (Kody Błędów UART / NMEA w Ramce 0x02)
**Plik:** `src/src/rx-serial/SerialGPS.cpp`

Gdy odbiornik nie zablokował jeszcze prawidłowych pakietów NMEA (`validPacketsCount == 0`) lub nastąpiła utrata strumienia (`now - lastValidPacketMs > 3000`), funkcja `sendQueuedData()` co 1000 ms wysyła ramkę diagnostyczną `0x02` (`CRSF_FRAMETYPE_GPS`):
- `latitude` = 0, `longitude` = 0
- `satellites_in_use` = Kod stanu:
  - **`0` (`NO_DATA`):** 0 bajtów na RX (CH3) – linia odłączona lub brak zasilania GPS
  - **`1` (`SCANNING_BAUD`):** Bajty przychodzą, auto-baud skanuje prędkości
  - **`2` (`CHECKSUM_FAIL`):** Bajty przychodzą, ale ramki nie przechodzą sumy kontrolnej
  - **`3` (`BAUD_LOCKED`):** Prędkość UART dopasowana, oczekiwanie na pełne ramki
  - **`4` (`LOST_TIMEOUT`):** Utrata sygnału GPS w locie/jeździe (> 3s)
- `groundspeed` = Testowany baudrate / 100 (1152 = 115200 bps, 96 = 9600 bps, 384 = 38400 bps, 576 = 57600 bps)
- `altitude` = Licznik odebranych surowych bajtów (`1000 + rawBytesCount % 10000`)
- `gps_heading` = Licznik błędów sumy kontrolnej NMEA (`csumErrors % 36000`)

Po odebraniu pierwszej poprawnej ramki NMEA tryb diagnostyczny automatycznie ustępuje miejsca rzeczywistej telemetrii nawigacyjnej.

---

## 🔨 Budowanie i Generowanie Wsadu

1. **Przebudowa WebUI (opcjonalnie przy zmianach w HTML):**
   ```bash
   cd ExpressLRS/src/html
   npm run build:sx128x-rx-8285
   ```

2. **Kompilacja bazowa:**
   ```bash
   cd ExpressLRS/src
   pio run -e Unified_ESP8285_2400_RX_via_WIFI
   ```

2. **Aplikacja konfiguracji sprzętowej:**
   Uruchom skrypt dołączający layout pinów RadioMaster ER5C V2:
   ```bash
   python build_gps_release.py
   ```

3. **Wynikowe pliki binarne:**
   - `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin` (dla programatora FTDI)
   - `ELRS_V4.1_RadioMaster_ER5Cv2_GPS_IMU.bin.gz` (dla aktualizacji OTA przez WebUI)
