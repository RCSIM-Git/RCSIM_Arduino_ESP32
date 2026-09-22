#pragma once
#include <Arduino.h>

// =================================================================================
// RCSIM - CRSF Protocol Definitions and CRC8 DVB-S2 Engine
// =================================================================================

#define CRSF_SYNC_BYTE_RADIO        0xEA  // Radio transmitter
#define CRSF_SYNC_BYTE_FC           0xC8  // Flight Controller
#define CRSF_SYNC_BYTE_RECEIVER     0xEC  // CRSF Receiver
#define CRSF_SYNC_BYTE_LUA          0xEF  // LUA Handset

// Typy ramek CRSF
#define CRSF_FRAMETYPE_GPS          0x02
#define CRSF_FRAMETYPE_VARIO        0x07
#define CRSF_FRAMETYPE_BATTERY      0x08
#define CRSF_FRAMETYPE_HEARTBEAT    0x0B
#define CRSF_FRAMETYPE_LINK_STATS   0x14
#define CRSF_FRAMETYPE_RC_CHANNELS  0x16
#define CRSF_FRAMETYPE_ATTITUDE     0x1E
#define CRSF_FRAMETYPE_FLIGHT_MODE  0x21

#define CRSF_MAX_FRAME_SIZE         64
#define CRSF_NUM_CHANNELS           16

// Zakresy kanałów CRSF (11-bit)
#define CRSF_CHANNEL_MIN            172    // ~988 us
#define CRSF_CHANNEL_MID            992    // ~1500 us
#define CRSF_CHANNEL_MAX            1811   // ~2012 us

// Struktura telemetrii GPS
struct CRSF_GPS_Data {
  int32_t latitude;     // stopnie * 1e7
  int32_t longitude;    // stopnie * 1e7
  uint16_t groundspeed; // km/h * 10
  uint16_t heading;     // stopnie * 100
  uint16_t altitude;    // metry + 1000m offset
  uint8_t satellites;   // liczba satelitów
};

// Struktura telemetrii Baterii
struct CRSF_Battery_Data {
  uint16_t voltage;     // mV * 100 (0.1V)
  uint16_t current;     // mA * 100 (0.1A)
  uint32_t capacity;    // zużyte mAh (24-bit w ramce)
  uint8_t remaining;    // procent 0-100%
};

// Struktura telemetrii IMU / Attitude
struct CRSF_Attitude_Data {
  int16_t pitch;        // radiany * 10000
  int16_t roll;         // radiany * 10000
  int16_t yaw;          // radiany * 10000
};

// Struktura Link Statistics
struct CRSF_LinkStats_Data {
  uint8_t uplink_rssi_1; // -dBm (np. 50 = -50dBm)
  uint8_t uplink_rssi_2;
  uint8_t uplink_link_quality; // 0 - 100%
  int8_t  uplink_snr;
  uint8_t active_antenna;
  uint8_t rf_mode;
  uint8_t uplink_tx_power;
  uint8_t downlink_rssi;
  uint8_t downlink_link_quality;
  int8_t  downlink_snr;
};

class CRSFParser {
private:
  // Tablica wstępnie wyliczonego CRC8 DVB-S2 (Wielomian 0xD5)
  static const uint8_t crc8_dvb_s2_table[256];

  enum State {
    STATE_WAIT_SYNC,
    STATE_READ_LEN,
    STATE_READ_TYPE,
    STATE_READ_PAYLOAD,
    STATE_READ_CRC
  };

  State _state;
  uint8_t _frameBuffer[CRSF_MAX_FRAME_SIZE];
  uint8_t _expectedLen;
  uint8_t _payloadIdx;
  uint8_t _frameType;

public:
  uint16_t channels[CRSF_NUM_CHANNELS];
  unsigned long lastValidChannelsTime;
  bool isArmed;

  CRSFParser() {
    _state = STATE_WAIT_SYNC;
    _expectedLen = 0;
    _payloadIdx = 0;
    _frameType = 0;
    lastValidChannelsTime = 0;
    isArmed = false;
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
      channels[i] = CRSF_CHANNEL_MID;
    }
  }

  // Obliczenie sumy kontrolnej CRC8 DVB-S2
  static uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
      crc = crc8_dvb_s2_table[crc ^ data[i]];
    }
    return crc;
  }

  // Konwersja surowej wartości 11-bit CRSF na impuls mikrosekund (us)
  static inline uint16_t crsfToUs(uint16_t crsfVal) {
    if (crsfVal < CRSF_CHANNEL_MIN) crsfVal = CRSF_CHANNEL_MIN;
    if (crsfVal > CRSF_CHANNEL_MAX) crsfVal = CRSF_CHANNEL_MAX;
    // Mapowanie: 172 -> 988us, 992 -> 1500us, 1811 -> 2012us
    return (uint16_t)(988 + (((int32_t)(crsfVal - CRSF_CHANNEL_MIN) * 1024) / (CRSF_CHANNEL_MAX - CRSF_CHANNEL_MIN)));
  }

  // Automat stanów (FSM) przetwarzający pojedynczy bajt
  // Zwraca typ ramki jeśli skompletowano i zweryfikowano pakiet, w przeciwnym razie 0
  uint8_t processByte(uint8_t b) {
    switch (_state) {
      case STATE_WAIT_SYNC:
        if (b == CRSF_SYNC_BYTE_FC || b == CRSF_SYNC_BYTE_RADIO || b == CRSF_SYNC_BYTE_RECEIVER) {
          _frameBuffer[0] = b;
          _state = STATE_READ_LEN;
        }
        break;

      case STATE_READ_LEN:
        // Długość obejmuje bajt typu, payload oraz bajt CRC
        if (b >= 2 && b <= (CRSF_MAX_FRAME_SIZE - 2)) {
          _expectedLen = b;
          _frameBuffer[1] = b;
          _state = STATE_READ_TYPE;
        } else {
          _state = STATE_WAIT_SYNC;
        }
        break;

      case STATE_READ_TYPE:
        _frameType = b;
        _frameBuffer[2] = b;
        _payloadIdx = 0;
        if (_expectedLen == 2) {
          // Ramka bez payloadu, od razu CRC
          _state = STATE_READ_CRC;
        } else {
          _state = STATE_READ_PAYLOAD;
        }
        break;

      case STATE_READ_PAYLOAD:
        _frameBuffer[3 + _payloadIdx] = b;
        _payloadIdx++;
        // Payload ma rozmiar: (_expectedLen - 2) bajtów
        if (_payloadIdx >= (_expectedLen - 2)) {
          _state = STATE_READ_CRC;
        }
        break;

      case STATE_READ_CRC:
        _state = STATE_WAIT_SYNC;
        // Oblicz CRC dla: TYPE (indeks 2) do końca PAYLOAD
        uint8_t calculatedCRC = crc8(&_frameBuffer[2], _expectedLen - 1);
        if (calculatedCRC == b) {
          if (_frameType == CRSF_FRAMETYPE_RC_CHANNELS) {
            unpackChannels(&_frameBuffer[3]);
            lastValidChannelsTime = millis();
            // Kanał 5 (AUX1 / Index 4) w standardzie ELRS: > 1500us to ARM
            isArmed = (crsfToUs(channels[4]) > 1350);
          }
          return _frameType;
        }
        break;
    }
    return 0;
  }

  // Rozpakowanie 22 bajtów na 16 kanałów 11-bitowych
  void unpackChannels(const uint8_t *payload) {
    channels[0]  = ((payload[0])       | (payload[1]  << 8)) & 0x07FF;
    channels[1]  = ((payload[1]  >> 3) | (payload[2]  << 5)) & 0x07FF;
    channels[2]  = ((payload[2]  >> 6) | (payload[3]  << 2) | (payload[4] << 10)) & 0x07FF;
    channels[3]  = ((payload[4]  >> 1) | (payload[5]  << 7)) & 0x07FF;
    channels[4]  = ((payload[5]  >> 4) | (payload[6]  << 4)) & 0x07FF;
    channels[5]  = ((payload[6]  >> 7) | (payload[7]  << 1) | (payload[8] << 9)) & 0x07FF;
    channels[6]  = ((payload[8]  >> 2) | (payload[9]  << 6)) & 0x07FF;
    channels[7]  = ((payload[9]  >> 5) | (payload[10] << 3)) & 0x07FF;
    channels[8]  = ((payload[11])      | (payload[12] << 8)) & 0x07FF;
    channels[9]  = ((payload[12] >> 3) | (payload[13] << 5)) & 0x07FF;
    channels[10] = ((payload[13] >> 6) | (payload[14] << 2) | (payload[15] << 10)) & 0x07FF;
    channels[11] = ((payload[15] >> 1) | (payload[16] << 7)) & 0x07FF;
    channels[12] = ((payload[16] >> 4) | (payload[17] << 4)) & 0x07FF;
    channels[13] = ((payload[17] >> 7) | (payload[18] << 1) | (payload[19] << 9)) & 0x07FF;
    channels[14] = ((payload[19] >> 2) | (payload[20] << 6)) & 0x07FF;
    channels[15] = ((payload[20] >> 5) | (payload[21] << 3)) & 0x07FF;
  }

  // Budowanie ramki telemetrii GPS (0x02) - zwraca całkowitą długość ramki
  static uint8_t packGPS(uint8_t *outBuf, const CRSF_GPS_Data &data) {
    outBuf[0] = CRSF_SYNC_BYTE_RADIO;
    outBuf[1] = 17; // Długość: 1 (Type) + 15 (Payload) + 1 (CRC)
    outBuf[2] = CRSF_FRAMETYPE_GPS;

    // Big-Endian zgodnie ze standardem CRSF
    outBuf[3]  = (data.latitude >> 24) & 0xFF;
    outBuf[4]  = (data.latitude >> 16) & 0xFF;
    outBuf[5]  = (data.latitude >> 8)  & 0xFF;
    outBuf[6]  = (data.latitude)       & 0xFF;

    outBuf[7]  = (data.longitude >> 24) & 0xFF;
    outBuf[8]  = (data.longitude >> 16) & 0xFF;
    outBuf[9]  = (data.longitude >> 8)  & 0xFF;
    outBuf[10] = (data.longitude)       & 0xFF;

    outBuf[11] = (data.groundspeed >> 8) & 0xFF;
    outBuf[12] = (data.groundspeed)      & 0xFF;

    outBuf[13] = (data.heading >> 8) & 0xFF;
    outBuf[14] = (data.heading)      & 0xFF;

    outBuf[15] = (data.altitude >> 8) & 0xFF;
    outBuf[16] = (data.altitude)      & 0xFF;

    outBuf[17] = data.satellites;

    outBuf[18] = crc8(&outBuf[2], 16);
    return 19;
  }

  // Budowanie ramki telemetrii Baterii (0x08)
  static uint8_t packBattery(uint8_t *outBuf, const CRSF_Battery_Data &data) {
    outBuf[0] = CRSF_SYNC_BYTE_RADIO;
    outBuf[1] = 10; // Długość: 1 + 8 + 1
    outBuf[2] = CRSF_FRAMETYPE_BATTERY;

    outBuf[3] = (data.voltage >> 8) & 0xFF;
    outBuf[4] = (data.voltage)      & 0xFF;

    outBuf[5] = (data.current >> 8) & 0xFF;
    outBuf[6] = (data.current)      & 0xFF;

    outBuf[7] = (data.capacity >> 16) & 0xFF;
    outBuf[8] = (data.capacity >> 8)  & 0xFF;
    outBuf[9] = (data.capacity)       & 0xFF;

    outBuf[10] = data.remaining;

    outBuf[11] = crc8(&outBuf[2], 9);
    return 12;
  }

  // Budowanie ramki telemetrii IMU / Attitude (0x1E)
  static uint8_t packAttitude(uint8_t *outBuf, const CRSF_Attitude_Data &data) {
    outBuf[0] = CRSF_SYNC_BYTE_RADIO;
    outBuf[1] = 8; // Długość: 1 + 6 + 1
    outBuf[2] = CRSF_FRAMETYPE_ATTITUDE;

    outBuf[3] = (data.pitch >> 8) & 0xFF;
    outBuf[4] = (data.pitch)      & 0xFF;

    outBuf[5] = (data.roll >> 8)  & 0xFF;
    outBuf[6] = (data.roll)       & 0xFF;

    outBuf[7] = (data.yaw >> 8)   & 0xFF;
    outBuf[8] = (data.yaw)        & 0xFF;

    outBuf[9] = crc8(&outBuf[2], 7);
    return 10;
  }

  // Budowanie ramki Link Statistics (0x14)
  static uint8_t packLinkStats(uint8_t *outBuf, const CRSF_LinkStats_Data &data) {
    outBuf[0] = CRSF_SYNC_BYTE_RADIO;
    outBuf[1] = 12; // Długość: 1 + 10 + 1
    outBuf[2] = CRSF_FRAMETYPE_LINK_STATS;

    outBuf[3]  = data.uplink_rssi_1;
    outBuf[4]  = data.uplink_rssi_2;
    outBuf[5]  = data.uplink_link_quality;
    outBuf[6]  = (uint8_t)data.uplink_snr;
    outBuf[7]  = data.active_antenna;
    outBuf[8]  = data.rf_mode;
    outBuf[9]  = data.uplink_tx_power;
    outBuf[10] = data.downlink_rssi;
    outBuf[11] = data.downlink_link_quality;
    outBuf[12] = (uint8_t)data.downlink_snr;

    outBuf[13] = crc8(&outBuf[2], 11);
    return 14;
  }
};

// Wstępnie obliczona tablica CRC8 DVB-S2 (Wielomian 0xD5)
const uint8_t CRSFParser::crc8_dvb_s2_table[256] = {
  0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
  0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
  0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
  0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
  0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
  0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
  0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
  0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
  0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
  0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
  0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
  0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
  0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
  0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
  0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
  0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};
