#pragma once
#include <Arduino.h>
#include "CRSFParser.h"
#include "CRSFTransport.h"

// =================================================================================
// RCSIM - CRSF Dual-Link Hybrid Multiplexer (Muxer)
// =================================================================================
// Zapewnia równoczesną obsługę dwóch niezależnych torów sterowania:
//   1. Link Lokalny RF (Hardware UART): Odbiornik ELRS (np. RadioMaster ER5C V2)
//   2. Link Sieciowy / Internet 5G: VPN Tailscale / Wi-Fi UDP (RCSIM PC GCS)
//
// Funkcjonalności:
//   - Automatyczny arbiter (Muxer) z priorytetem lokalnego radia RF i płynnym
//     przełączaniem na 5G w przypadku utraty zasięgu lub wyłączenia nadajnika MT12.
//   - Sprzętowy Fail-Safe (150 ms) przy utracie obu torów łączności.
//   - Opcjonalne przełączanie trybu za pomocą trójpozycyjnego przełącznika na aparaturze
//     (np. AUX2 / CH6: Góra = Wymuś RF, Środek = AUTO, Dół = Wymuś 5G VPN).
//   - Obustronny dispatch telemetrii (Attitude, Bateria, GPS, LinkStats): wysyłana
//     jednocześnie po kablu UART do ER5c V2 (ekran MT12) oraz przez VPN do RCSIM GCS.
// =================================================================================

enum LinkSource {
  LINK_SOURCE_NONE = 0,
  LINK_SOURCE_RF   = 1,   // RadioMaster MT12 + ER5C V2 ELRS
  LINK_SOURCE_NET  = 2    // 5G Internet / Tailscale VPN / UDP
};

enum MuxerMode {
  MUX_MODE_AUTO      = 0, // Domyślny: Priorytet RF, fallback na 5G
  MUX_MODE_FORCE_RF  = 1, // Wymuś sterowanie wyłącznie z lokalnego radia
  MUX_MODE_FORCE_NET = 2  // Wymuś sterowanie wyłącznie przez Internet 5G
};

struct DualLinkStats {
  LinkSource activeSource;
  MuxerMode currentMode;
  bool rfAlive;
  bool netAlive;
  uint32_t lastRfPacketAgeMs;
  uint32_t lastNetPacketAgeMs;
  uint32_t totalRfPackets;
  uint32_t totalNetPackets;
  uint32_t switchCount;
};

class CRSFDualLinkMuxer {
private:
  HardwareSerial &_rfSerial;
  CRSFTransport  &_netTransport;

  CRSFParser _rfParser;
  CRSFParser _netParser;

  uint32_t _rfBaud;
  int8_t   _rfRxPin;
  int8_t   _rfTxPin;
  uint32_t _timeoutMs;
  int8_t   _modeSwitchCh; // Kanał do przełączania trybu (np. 5 dla CH6/AUX2, -1 jeśli wyłączony)

  MuxerMode  _configuredMode;
  LinkSource _activeSource;
  uint32_t   _totalRfPackets;
  uint32_t   _totalNetPackets;
  uint32_t   _switchCount;

  // Histereza powrotu na RF (liczba poprawnych kolejnych ramek RF wymagana do przełączenia)
  uint8_t  _rfStableFrameCount;
  static const uint8_t RF_STABLE_THRESHOLD = 3;

public:
  CRSFDualLinkMuxer(
    HardwareSerial &rfSerial,
    CRSFTransport  &netTransport,
    uint32_t timeoutMs = 150,
    int8_t modeSwitchCh = 5 // Domyślnie Kanał 6 (indeks 5, AUX2)
  ) : _rfSerial(rfSerial), _netTransport(netTransport),
      _rfBaud(420000), _rfRxPin(16), _rfTxPin(17),
      _timeoutMs(timeoutMs), _modeSwitchCh(modeSwitchCh),
      _configuredMode(MUX_MODE_AUTO), _activeSource(LINK_SOURCE_NONE),
      _totalRfPackets(0), _totalNetPackets(0), _switchCount(0),
      _rfStableFrameCount(0) {}

  bool begin(uint32_t baud = 420000, int8_t rxPin = 16, int8_t txPin = 17) {
    _rfBaud = baud;
    _rfRxPin = rxPin;
    _rfTxPin = txPin;

    Serial.printf("[DualMuxer] Inicjalizacja portu RF CRSF UART (Baud: %u, RX: %d, TX: %d)...\n",
                  _rfBaud, _rfRxPin, _rfTxPin);

    // Inicjalizacja sprzętowego UART dla odbiornika ELRS (standard CRSF: 420 000 baud, 8N1)
    if (_rfRxPin >= 0 && _rfTxPin >= 0) {
      _rfSerial.begin(_rfBaud, SERIAL_8N1, _rfRxPin, _rfTxPin);
    } else {
      _rfSerial.begin(_rfBaud);
    }

    // Inicjalizacja warstwy sieciowej (5G / VPN / UDP)
    bool netOk = _netTransport.begin();
    if (!netOk) {
      Serial.println("[DualMuxer] OSTRZEŻENIE: Warstwa sieciowa (5G/VPN) zgłosiła brak gotowości (możliwy brak Wi-Fi).");
    }

    Serial.println("[DualMuxer] Hub Dual-Link gotowy do pracy w trybie hybrydowym.");
    return true;
  }

  // Główna pętla arbitrażu wywoływana w Core 0
  void update() {
    // 1. Odbiór bajtów z toru radiowego RF (Hardware UART - ER5C V2)
    while (_rfSerial.available() > 0) {
      uint8_t b = _rfSerial.read();
      uint8_t frameType = _rfParser.processByte(b);
      if (frameType == CRSF_FRAMETYPE_RC_CHANNELS) {
        _totalRfPackets++;
        if (_rfStableFrameCount < RF_STABLE_THRESHOLD) {
          _rfStableFrameCount++;
        }
      }
    }

    // 2. Odbiór bajtów z toru sieciowego (5G Internet / Tailscale VPN / UDP)
    while (_netTransport.available() > 0) {
      uint8_t b = _netTransport.read();
      uint8_t frameType = _netParser.processByte(b);
      if (frameType == CRSF_FRAMETYPE_RC_CHANNELS) {
        _totalNetPackets++;
      }
    }

    // 3. Ocena żywotności (Liveness) obu linków
    unsigned long now = millis();
    bool rfAlive  = (_rfParser.lastValidChannelsTime > 0) &&
                    ((now - _rfParser.lastValidChannelsTime) < _timeoutMs);
    bool netAlive = (_netParser.lastValidChannelsTime > 0) &&
                    ((now - _netParser.lastValidChannelsTime) < _timeoutMs);

    if (!rfAlive) {
      _rfStableFrameCount = 0;
    }

    // 4. Odczyt przełącznika trybów (AUX Switch), jeśli skonfigurowany
    MuxerMode effectiveMode = _configuredMode;
    if (_modeSwitchCh >= 0 && _modeSwitchCh < CRSF_NUM_CHANNELS) {
      uint16_t swVal = 1500;
      if (_activeSource == LINK_SOURCE_RF || rfAlive) {
        swVal = CRSFParser::crsfToUs(_rfParser.channels[_modeSwitchCh]);
      } else if (_activeSource == LINK_SOURCE_NET || netAlive) {
        swVal = CRSFParser::crsfToUs(_netParser.channels[_modeSwitchCh]);
      }

      if (swVal < 1300) {
        effectiveMode = MUX_MODE_FORCE_RF;
      } else if (swVal > 1700) {
        effectiveMode = MUX_MODE_FORCE_NET;
      } else {
        effectiveMode = MUX_MODE_AUTO;
      }
    }

    // 5. Logika arbitrażu (Wybór aktywnego źródła sygnału)
    LinkSource prevSource = _activeSource;

    switch (effectiveMode) {
      case MUX_MODE_FORCE_RF:
        _activeSource = rfAlive ? LINK_SOURCE_RF : LINK_SOURCE_NONE;
        break;

      case MUX_MODE_FORCE_NET:
        _activeSource = netAlive ? LINK_SOURCE_NET : LINK_SOURCE_NONE;
        break;

      case MUX_MODE_AUTO:
      default:
        // Priorytet RF z histerezą stabilności
        if (rfAlive && (_rfStableFrameCount >= RF_STABLE_THRESHOLD || _activeSource == LINK_SOURCE_RF)) {
          _activeSource = LINK_SOURCE_RF;
        } else if (netAlive) {
          _activeSource = LINK_SOURCE_NET;
        } else {
          _activeSource = LINK_SOURCE_NONE;
        }
        break;
    }

    // Wykrycie zmiany źródła
    if (_activeSource != prevSource) {
      _switchCount++;
      const char *srcName = (_activeSource == LINK_SOURCE_RF) ? "LOKALNY RF (MT12)" :
                            (_activeSource == LINK_SOURCE_NET) ? "INTERNET 5G (VPN/PC)" : "BRAK (FAILSAFE)";
      Serial.printf("[DualMuxer] Przełączenie źródła sterowania na: %s\n", srcName);
    }
  }

  // Pobranie aktualnego stanu kanałów sterujących, uzbrojenia i flagi Fail-Safe
  bool getControlData(uint16_t outPulseUs[CRSF_NUM_CHANNELS], bool &outArmed, bool &outFailsafe, LinkSource &outSource) {
    outSource = _activeSource;

    if (_activeSource == LINK_SOURCE_RF) {
      outArmed = _rfParser.isArmed;
      outFailsafe = false;
      for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        outPulseUs[i] = CRSFParser::crsfToUs(_rfParser.channels[i]);
      }
      return true;
    } else if (_activeSource == LINK_SOURCE_NET) {
      outArmed = _netParser.isArmed;
      outFailsafe = false;
      for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        outPulseUs[i] = CRSFParser::crsfToUs(_netParser.channels[i]);
      }
      return true;
    } else {
      // LINK_SOURCE_NONE -> Twardy Fail-Safe
      outArmed = false;
      outFailsafe = true;
      for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        outPulseUs[i] = 1500; // Pozycja neutralna
      }
      return false;
    }
  }

  // Jednoczesna wysyłka telemetrii do obu mediów
  void sendTelemetry(const uint8_t *buffer, size_t size) {
    if (!buffer || size == 0) return;

    // 1. Wysyłka do lokalnego odbiornika ELRS przez UART (dzięki temu MT12 ma telemetrię)
    _rfSerial.write(buffer, size);

    // 2. Wysyłka przez sieć 5G / VPN / UDP do stacji RCSIM na PC
    _netTransport.write(buffer, size);
  }

  // Pobranie statystyk działania Muxera
  void getStats(DualLinkStats &stats) {
    unsigned long now = millis();
    stats.activeSource = _activeSource;
    stats.currentMode = _configuredMode;
    stats.rfAlive = (_rfParser.lastValidChannelsTime > 0) && ((now - _rfParser.lastValidChannelsTime) < _timeoutMs);
    stats.netAlive = (_netParser.lastValidChannelsTime > 0) && ((now - _netParser.lastValidChannelsTime) < _timeoutMs);
    stats.lastRfPacketAgeMs = (_rfParser.lastValidChannelsTime > 0) ? (now - _rfParser.lastValidChannelsTime) : 999999;
    stats.lastNetPacketAgeMs = (_netParser.lastValidChannelsTime > 0) ? (now - _netParser.lastValidChannelsTime) : 999999;
    stats.totalRfPackets = _totalRfPackets;
    stats.totalNetPackets = _totalNetPackets;
    stats.switchCount = _switchCount;
  }

  void setMode(MuxerMode mode) {
    _configuredMode = mode;
  }

  LinkSource getActiveSource() const {
    return _activeSource;
  }

  CRSFParser& getRfParser() { return _rfParser; }
  CRSFParser& getNetParser() { return _netParser; }
};
