#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include "CRSFParser.h"

// =================================================================================
// RCSIM - Sensors Manager (IMU, GPS, ADC Battery)
// =================================================================================

#define IMU_NONE             0
#define IMU_MPU6050          1
#define IMU_MPU9250          2
#define IMU_BNO08X           3

#ifndef ACTIVE_IMU
#define ACTIVE_IMU           IMU_MPU6050
#endif

#if (ACTIVE_IMU == IMU_MPU6050)
  #include <MPU6050_light.h>
#endif

class SensorsManager {
private:
  HardwareSerial &_gpsSerial;
  TinyGPSPlus _gps;
  uint8_t _vbatPin;
  float _dividerRatio;
  float _filteredVbat;

  #if (ACTIVE_IMU == IMU_MPU6050)
  MPU6050 _mpu;
  #endif

  bool _imuReady;
  bool _gpsReady;

public:
  SensorsManager(HardwareSerial &gpsSerial, uint8_t vbatPin = 33, float dividerRatio = 5.545f)
    : _gpsSerial(gpsSerial), _vbatPin(vbatPin), _dividerRatio(dividerRatio),
      _filteredVbat(0.0f),
      #if (ACTIVE_IMU == IMU_MPU6050)
      _mpu(Wire),
      #endif
      _imuReady(false), _gpsReady(false) {}

  bool begin(uint32_t gpsBaud = 9600, int8_t gpsRxPin = 32, int8_t gpsTxPin = -1) {
    // 1. Inicjalizacja ADC1
    analogReadResolution(12);
    pinMode(_vbatPin, INPUT);
    _filteredVbat = readRawBatteryVoltage();

    // 2. Inicjalizacja GPS UART
    if (gpsRxPin >= 0) {
      _gpsSerial.begin(gpsBaud, SERIAL_8N1, gpsRxPin, gpsTxPin);
      _gpsReady = true;
      Serial.println("[Sensors] GPS UART zainicjalizowany.");
    }

    // 3. Inicjalizacja IMU
    #if (ACTIVE_IMU == IMU_MPU6050)
    byte status = _mpu.begin();
    if (status == 0) {
      _mpu.calcOffsets();
      // Konfiguracja sprzętowego filtra dolnoprzepustowego DLPF (~42 Hz)
      Wire.beginTransmission(0x68);
      Wire.write(0x1A); // Rejestr CONFIG
      Wire.write(0x03); // DLPF_CFG = 3 (Akcelerometr ~44Hz, Żyroskop ~42Hz)
      Wire.endTransmission();
      _imuReady = true;
      Serial.println("[Sensors] MPU6050 zainicjalizowany z filtrem DLPF.");
    } else {
      Serial.printf("[Sensors] BŁĄD inicjalizacji MPU6050 (kod: %d)\n", status);
    }
    #endif

    return true;
  }

  // Przetwarzanie strumienia NMEA z GPS (wywoływane w każdej pętli)
  void updateGPS() {
    if (!_gpsReady) return;
    while (_gpsSerial.available() > 0) {
      _gps.encode(_gpsSerial.read());
    }
  }

  // Pobranie danych IMU i przygotowanie struktury CRSF_Attitude_Data
  bool getAttitudeCRSF(CRSF_Attitude_Data &att) {
    #if (ACTIVE_IMU == IMU_MPU6050)
    if (_imuReady) {
      _mpu.update();
      float rollDeg  = _mpu.getAngleX();
      float pitchDeg = _mpu.getAngleY();
      float yawDeg   = _mpu.getAngleZ();

      // Przeliczenie stopni na radiany * 10000 (standard CRSF Attitude)
      const float DEG_TO_RAD_10000 = (3.14159265f / 180.0f) * 10000.0f;
      att.pitch = (int16_t)(pitchDeg * DEG_TO_RAD_10000);
      att.roll  = (int16_t)(rollDeg * DEG_TO_RAD_10000);
      att.yaw   = (int16_t)(yawDeg * DEG_TO_RAD_10000);
      return true;
    }
    #endif

    att.pitch = 0;
    att.roll = 0;
    att.yaw = 0;
    return false;
  }

  // Pobranie danych GPS i przygotowanie struktury CRSF_GPS_Data
  bool getGPSCRSF(CRSF_GPS_Data &gpsData) {
    if (_gps.location.isValid()) {
      gpsData.latitude    = (int32_t)(_gps.location.lat() * 1e7);
      gpsData.longitude   = (int32_t)(_gps.location.lng() * 1e7);
      gpsData.groundspeed = (uint16_t)(_gps.speed.kmph() * 10.0f);
      gpsData.heading     = (uint16_t)(_gps.course.deg() * 100.0f);
      gpsData.altitude    = (uint16_t)(_gps.altitude.meters() + 1000.0f); // Offset 1000m
      gpsData.satellites  = (uint8_t)(_gps.satellites.isValid() ? _gps.satellites.value() : 0);
      return true;
    }
    return false;
  }

  // Odczyt napięcia akumulatora z filtrem EMA
  float readRawBatteryVoltage() {
    int raw = analogRead(_vbatPin);
    float pinVolts = (raw / 4095.0f) * 3.3f;
    return pinVolts * _dividerRatio;
  }

  float getBatteryVoltage() {
    float raw = readRawBatteryVoltage();
    // Filtr dolnoprzepustowy IIR / EMA (alfa = 0.1)
    _filteredVbat = (_filteredVbat * 0.9f) + (raw * 0.1f);
    return _filteredVbat;
  }

  // Pobranie struktury CRSF_Battery_Data
  void getBatteryCRSF(CRSF_Battery_Data &batData) {
    float v = getBatteryVoltage();
    batData.voltage = (uint16_t)(v * 10.0f); // 0.1 V
    batData.current = 0;                     // Opcjonalnie bocznik prądowy
    batData.capacity = 0;
    // Oszacowanie procentowe dla pakietu 3S LiPo (9.9V - 12.6V)
    float pct = ((v - 9.9f) / (12.6f - 9.9f)) * 100.0f;
    batData.remaining = (uint8_t)constrain(pct, 0.0f, 100.0f);
  }

  bool isIMUReady() const { return _imuReady; }
  bool isGPSFix() { return _gps.location.isValid(); }
};
