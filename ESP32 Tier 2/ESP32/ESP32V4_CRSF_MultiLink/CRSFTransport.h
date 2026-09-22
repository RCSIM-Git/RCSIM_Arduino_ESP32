#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WiFiUdp.h>

// =================================================================================
// RCSIM - CRSF Transport Layer Abstraction
// Wsparcie dla: ESP-NOW, Hardware UART/Serial, UDP (Wi-Fi/GSM)
// =================================================================================

enum TransportMode {
  TRANSPORT_ESP_NOW,
  TRANSPORT_SERIAL,
  TRANSPORT_UDP
};

class CRSFTransport {
public:
  virtual bool begin() = 0;
  virtual int available() = 0;
  virtual uint8_t read() = 0;
  virtual size_t write(const uint8_t *buffer, size_t size) = 0;
  virtual void loop() {}
  virtual int8_t getRSSI() { return -50; }
  virtual uint8_t getLinkQuality() { return 100; }
};

// ---------------------------------------------------------------------------------
// 1. ESP-NOW Transport (Niskie opóźnienia 1-2 ms, p2p warstwy MAC)
// ---------------------------------------------------------------------------------
#define ESPNOW_RING_BUF_SIZE 512

class ESPNowTransport : public CRSFTransport {
private:
  uint8_t _peerMac[6];
  uint8_t _rxBuffer[ESPNOW_RING_BUF_SIZE];
  volatile uint16_t _rxHead;
  volatile uint16_t _rxTail;
  int8_t _lastRssi;
  uint32_t _packetsReceived;
  uint32_t _lastPacketTime;

  static ESPNowTransport *_instance;

  static void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (_instance) {
      _instance->handleIncoming(mac, incomingData, len);
    }
  }

  void handleIncoming(const uint8_t *mac, const uint8_t *incomingData, int len) {
    _lastPacketTime = millis();
    _packetsReceived++;
    for (int i = 0; i < len; i++) {
      uint16_t nextHead = (_rxHead + 1) % ESPNOW_RING_BUF_SIZE;
      if (nextHead != _rxTail) {
        _rxBuffer[_rxHead] = incomingData[i];
        _rxHead = nextHead;
      }
    }
  }

public:
  ESPNowTransport(const uint8_t *peerMac = nullptr) {
    _instance = this;
    _rxHead = 0;
    _rxTail = 0;
    _lastRssi = -50;
    _packetsReceived = 0;
    _lastPacketTime = 0;
    if (peerMac) {
      memcpy(_peerMac, peerMac, 6);
    } else {
      // Domyślny Broadcast
      memset(_peerMac, 0xFF, 6);
    }
  }

  bool begin() override {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
      Serial.println("[ESP-NOW] Błąd inicjalizacji!");
      return false;
    }

    esp_now_register_recv_cb(onDataRecv);

    // Rejestracja peera
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, _peerMac, 6);
    peerInfo.channel = 0; // Bieżący kanał Wi-Fi
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("[ESP-NOW] Ostrzeżenie: Dodawanie peera zakończone statusem różnym od OK");
    }

    Serial.print("[ESP-NOW] Gotowy. MAC urządzenia: ");
    Serial.println(WiFi.macAddress());
    return true;
  }

  int available() override {
    if (_rxHead >= _rxTail) {
      return _rxHead - _rxTail;
    }
    return ESPNOW_RING_BUF_SIZE - _rxTail + _rxHead;
  }

  uint8_t read() override {
    if (_rxHead == _rxTail) return 0;
    uint8_t b = _rxBuffer[_rxTail];
    _rxTail = (_rxTail + 1) % ESPNOW_RING_BUF_SIZE;
    return b;
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    if (size > 250) size = 250; // Limit ramki ESP-NOW
    esp_err_t result = esp_now_send(_peerMac, buffer, size);
    return (result == ESP_OK) ? size : 0;
  }

  int8_t getRSSI() override {
    return (millis() - _lastPacketTime < 1000) ? -55 : -99;
  }

  uint8_t getLinkQuality() override {
    return (millis() - _lastPacketTime < 250) ? 100 : 0;
  }
};

ESPNowTransport *ESPNowTransport::_instance = nullptr;

// ---------------------------------------------------------------------------------
// 2. Hardware Serial Transport (USB CDC / UART do zewnętrznego LoRa / PC)
// ---------------------------------------------------------------------------------
class SerialTransport : public CRSFTransport {
private:
  HardwareSerial &_serial;
  uint32_t _baud;
  int8_t _rxPin;
  int8_t _txPin;

public:
  SerialTransport(HardwareSerial &serial, uint32_t baud = 115200, int8_t rxPin = -1, int8_t txPin = -1)
    : _serial(serial), _baud(baud), _rxPin(rxPin), _txPin(txPin) {}

  bool begin() override {
    if (_rxPin >= 0 && _txPin >= 0) {
      _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    } else {
      _serial.begin(_baud);
    }
    return true;
  }

  int available() override {
    return _serial.available();
  }

  uint8_t read() override {
    return _serial.read();
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    return _serial.write(buffer, size);
  }
};

// ---------------------------------------------------------------------------------
// 3. UDP Transport (Tradycyjne Wi-Fi / Router / Moduł GSM/LTE Bridge)
// ---------------------------------------------------------------------------------
class UDPTransport : public CRSFTransport {
private:
  WiFiUDP _udp;
  uint16_t _localPort;
  IPAddress _remoteIP;
  uint16_t _remotePort;
  uint8_t _packetBuf[256];
  int _packetLen;
  int _packetPos;

public:
  UDPTransport(uint16_t localPort = 12345, uint16_t remotePort = 12347)
    : _localPort(localPort), _remotePort(remotePort), _packetLen(0), _packetPos(0) {}

  bool begin() override {
    return _udp.begin(_localPort);
  }

  int available() override {
    if (_packetPos < _packetLen) {
      return _packetLen - _packetPos;
    }
    _packetLen = _udp.parsePacket();
    if (_packetLen > 0) {
      _remoteIP = _udp.remoteIP();
      if (_packetLen > (int)sizeof(_packetBuf)) _packetLen = sizeof(_packetBuf);
      _udp.read(_packetBuf, _packetLen);
      _packetPos = 0;
      return _packetLen;
    }
    return 0;
  }

  uint8_t read() override {
    if (_packetPos < _packetLen) {
      return _packetBuf[_packetPos++];
    }
    return 0;
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    if (!_remoteIP) return 0;
    _udp.beginPacket(_remoteIP, _remotePort);
    size_t written = _udp.write(buffer, size);
    _udp.endPacket();
    return written;
  }

  void setRemoteTarget(IPAddress ip, uint16_t port) {
    _remoteIP = ip;
    _remotePort = port;
  }

  int8_t getRSSI() override {
    return WiFi.RSSI();
  }
};
