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
        bool txIsPwm = false;
        #if defined(GPIO_PIN_PWM_OUTPUTS_COUNT)
        for (int ch = 0; ch < GPIO_PIN_PWM_OUTPUTS_COUNT; ++ch)
        {
            if (GPIO_PIN_PWM_OUTPUTS[ch] == 1 && config.GetPwmChannel(ch)->val.mode < somSerial)
            {
                txIsPwm = true;
                break;
            }
        }
        #endif
        if (txIsPwm)
        {
            mode = SERIAL_RX_ONLY;
        }
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

## 🔨 Budowanie i Generowanie Wsadu

1. **Kompilacja bazowa:**
   ```bash
   cd ExpressLRS
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
