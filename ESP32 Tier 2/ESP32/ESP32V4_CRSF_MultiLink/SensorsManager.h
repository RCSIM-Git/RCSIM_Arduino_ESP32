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
#define IMU_LSM6DS3          4

#ifndef ACTIVE_IMU
#define ACTIVE_IMU           IMU_LSM6DS3
#endif

#if (ACTIVE_IMU == IMU_MPU6050)
  #include <MPU6050_light.h>
#endif

// Adresy I2C
#define TCA9548A_I2C_ADDR    0x70
#define LSM6DS3_I2C_ADDR_A   0x6A // Domyślny adres Grove LSM6DS3
#define LSM6DS3_I2C_ADDR_B   0x6B // Alternatywny adres

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

  // Stan LSM6DS3 (Grove IMU)
  uint8_t  _lsmAddr;
  int16_t  _gyroBiasX, _gyroBiasY, _gyroBiasZ;
  float    _rollDeg, _pitchDeg, _yawDeg;
  uint32_t _lastImuMicros;

  bool _imuReady;
  bool _gpsReady;
  bool _tcaDetected;

public:
  SensorsManager(HardwareSerial &gpsSerial, uint8_t vbatPin = 1, float dividerRatio = 5.545f)
    : _gpsSerial(gpsSerial), _vbatPin(vbatPin), _dividerRatio(dividerRatio),
      _filteredVbat(0.0f),
      #if (ACTIVE_IMU == IMU_MPU6050)
      _mpu(Wire),
      #endif
      _lsmAddr(LSM6DS3_I2C_ADDR_A),
      _gyroBiasX(0), _gyroBiasY(0), _gyroBiasZ(0),
      _rollDeg(0.0f), _pitchDeg(0.0f), _yawDeg(0.0f),
      _lastImuMicros(0),
      _imuReady(false), _gpsReady(false), _tcaDetected(false) {}

  // Aktywacja wszystkich lub wybranych portów huba TCA9548A
  static bool enableTCA9548A(uint8_t mask = 0xFF, uint8_t addr = TCA9548A_I2C_ADDR) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Wire.beginTransmission(addr);
      Wire.write(mask); // 0xFF włącza wszystkie 8 kanałów Grove równolegle
      Wire.endTransmission();
      return true;
    }
    return false;
  }

  bool begin(uint32_t gpsBaud = 9600, int8_t gpsRxPin = 18, int8_t gpsTxPin = 17) {
    // 1. Sprawdzenie i aktywacja huba Grove TCA9548A
    if (enableTCA9548A(0xFF)) {
      _tcaDetected = true;
      Serial.println("[Sensors] Wykryto Grove TCA9548A (0x70) - wszystkie 8 portów aktywne.");
    }

    // 2. Inicjalizacja ADC1 dla baterii (bezpieczne piny GPIO 1-10 na ESP32-S3)
    analogReadResolution(12);
    pinMode(_vbatPin, INPUT);
    _filteredVbat = readRawBatteryVoltage();

    // 3. Inicjalizacja GPS UART (Grove Air530)
    if (gpsRxPin >= 0) {
      _gpsSerial.begin(gpsBaud, SERIAL_8N1, gpsRxPin, gpsTxPin);
      _gpsReady = true;
      Serial.printf("[Sensors] Grove Air530 GPS UART zainicjalizowany (Baud: %u, RX: %d, TX: %d).\n",
                    gpsBaud, gpsRxPin, gpsTxPin);
    }

    // 4. Inicjalizacja IMU
    #if (ACTIVE_IMU == IMU_LSM6DS3)
    initLSM6DS3();
    #elif (ACTIVE_IMU == IMU_MPU6050)
    byte status = _mpu.begin();
    if (status == 0) {
      _mpu.calcOffsets();
      Wire.beginTransmission(0x68);
      Wire.write(0x1A); // Rejestr CONFIG
      Wire.write(0x03); // DLPF_CFG = 3
      Wire.endTransmission();
      _imuReady = true;
      Serial.println("[Sensors] MPU6050 zainicjalizowany z filtrem DLPF.");
    } else {
      Serial.printf("[Sensors] BŁĄD inicjalizacji MPU6050 (kod: %d)\n", status);
    }
    #endif

    return true;
  }

  // Wbudowany, lekki sterownik I2C dla Grove LSM6DS3 (bez zewnętrznych bibliotek)
  bool initLSM6DS3() {
    _lsmAddr = LSM6DS3_I2C_ADDR_A;
    Wire.beginTransmission(_lsmAddr);
    if (Wire.endTransmission() != 0) {
      _lsmAddr = LSM6DS3_I2C_ADDR_B;
      Wire.beginTransmission(_lsmAddr);
      if (Wire.endTransmission() != 0) {
        Serial.println("[Sensors] BŁĄD: Grove LSM6DS3 nie odpowiada pod 0x6A ani 0x6B!");
        return false;
      }
    }

    // Odczyt WHO_AM_I (rejestr 0x0F, oczekiwana wartość 0x69)
    Wire.beginTransmission(_lsmAddr);
    Wire.write(0x0F);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)_lsmAddr, (uint8_t)1);
    uint8_t who = Wire.available() ? Wire.read() : 0;
    if (who != 0x69) {
      Serial.printf("[Sensors] OSTRZEŻENIE: LSM6DS3 WHO_AM_I = 0x%02X (oczekiwano 0x69)\n", who);
    }

    // Konfiguracja rejestrów LSM6DS3:
    // 1. CTRL1_XL (0x10): ODR = 104 Hz (0x40), Zakres = +/- 4g (0x08) -> 0x48
    writeRegister(_lsmAddr, 0x10, 0x48);
    // 2. CTRL2_G  (0x11): ODR = 104 Hz (0x40), Zakres = 2000 dps (0x0C) -> 0x4C
    writeRegister(_lsmAddr, 0x11, 0x4C);
    // 3. CTRL3_C  (0x12): BDU=1 (Block Data Update), IF_INC=1 (Auto-increment) -> 0x44
    writeRegister(_lsmAddr, 0x12, 0x44);

    delay(20);

    // Szybka autokalibracja żyroskopu (zbieranie 64 próbek w stanie spoczynku)
    int32_t sumGx = 0, sumGy = 0, sumGz = 0;
    for (int i = 0; i < 64; i++) {
      int16_t gx, gy, gz, ax, ay, az;
      readLSM6DS3Raw(gx, gy, gz, ax, ay, az);
      sumGx += gx;
      sumGy += gy;
      sumGz += gz;
      delay(4);
    }
    _gyroBiasX = (int16_t)(sumGx / 64);
    _gyroBiasY = (int16_t)(sumGy / 64);
    _gyroBiasZ = (int16_t)(sumGz / 64);

    _lastImuMicros = micros();
    _imuReady = true;
    Serial.printf("[Sensors] Grove LSM6DS3 skalibrowany (Bias: X=%d, Y=%d, Z=%d).\n",
                  _gyroBiasX, _gyroBiasY, _gyroBiasZ);
    return true;
  }

  void writeRegister(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
  }

  bool readLSM6DS3Raw(int16_t &gx, int16_t &gy, int16_t &gz, int16_t &ax, int16_t &ay, int16_t &az) {
    // Odczyt ciągły 12 bajtów z rejestrów OUTX_L_G (0x22) do OUTZ_H_XL (0x2D)
    Wire.beginTransmission(_lsmAddr);
    Wire.write(0x22);
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint8_t)_lsmAddr, (uint8_t)12) != 12) return false;

    uint8_t gxl = Wire.read(); uint8_t gxh = Wire.read();
    uint8_t gyl = Wire.read(); uint8_t gyh = Wire.read();
    uint8_t gzl = Wire.read(); uint8_t gzh = Wire.read();

    uint8_t axl = Wire.read(); uint8_t axh = Wire.read();
    uint8_t ayl = Wire.read(); uint8_t ayh = Wire.read();
    uint8_t azl = Wire.read(); uint8_t azh = Wire.read();

    gx = (int16_t)((gxh << 8) | gxl);
    gy = (int16_t)((gyh << 8) | gyl);
    gz = (int16_t)((gzh << 8) | gzl);

    ax = (int16_t)((axh << 8) | axl);
    ay = (int16_t)((ayh << 8) | ayl);
    az = (int16_t)((azh << 8) | azl);
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
    #if (ACTIVE_IMU == IMU_LSM6DS3)
    if (_imuReady) {
      int16_t gxRaw, gyRaw, gzRaw, axRaw, ayRaw, azRaw;
      if (readLSM6DS3Raw(gxRaw, gyRaw, gzRaw, axRaw, ayRaw, azRaw)) {
        uint32_t nowMicros = micros();
        float dt = (nowMicros - _lastImuMicros) / 1000000.0f;
        if (dt <= 0.0f || dt > 0.1f) dt = 0.02f; // Zabezpieczenie przed skokami timera
        _lastImuMicros = nowMicros;

        // Przeliczenie żyroskopu: 2000 dps -> 70.0 mdps/LSB = 0.070 dps/LSB
        float rateX = (gxRaw - _gyroBiasX) * 0.070f;
        float rateY = (gyRaw - _gyroBiasY) * 0.070f;
        float rateZ = (gzRaw - _gyroBiasZ) * 0.070f;

        // Przeliczenie akcelerometru: +/- 4g -> 0.122 mg/LSB
        float axG = axRaw * 0.000122f;
        float ayG = ayRaw * 0.000122f;
        float azG = azRaw * 0.000122f;

        // Kąty z akcelerometru
        float rollAcc  = atan2(ayG, azG) * (180.0f / 3.14159265f);
        float pitchAcc = atan2(-axG, sqrt(ayG * ayG + azG * azG)) * (180.0f / 3.14159265f);

        // Filtr komplementarny (96% żyroskop, 4% akcelerometr)
        _rollDeg  = 0.96f * (_rollDeg + rateX * dt) + 0.04f * rollAcc;
        _pitchDeg = 0.96f * (_pitchDeg + rateY * dt) + 0.04f * pitchAcc;
        _yawDeg  += rateZ * dt;

        // Przeliczenie stopni na radiany * 10000 (standard CRSF Attitude 0x1E)
        const float DEG_TO_RAD_10000 = (3.14159265f / 180.0f) * 10000.0f;
        att.pitch = (int16_t)(_pitchDeg * DEG_TO_RAD_10000);
        att.roll  = (int16_t)(_rollDeg * DEG_TO_RAD_10000);
        att.yaw   = (int16_t)(_yawDeg * DEG_TO_RAD_10000);
        return true;
      }
    }
    #elif (ACTIVE_IMU == IMU_MPU6050)
    if (_imuReady) {
      _mpu.update();
      float rollDeg  = _mpu.getAngleX();
      float pitchDeg = _mpu.getAngleY();
      float yawDeg   = _mpu.getAngleZ();

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
    _filteredVbat = (_filteredVbat * 0.9f) + (raw * 0.1f);
    return _filteredVbat;
  }

  // Pobranie struktury CRSF_Battery_Data
  void getBatteryCRSF(CRSF_Battery_Data &batData) {
    float v = getBatteryVoltage();
    batData.voltage = (uint16_t)(v * 10.0f); // 0.1 V
    batData.current = 0;
    batData.capacity = 0;
    // Oszacowanie procentowe dla pakietu 4S LiPo (ARRMA Mojave 4S: 13.2V - 16.8V)
    float pct = ((v - 13.2f) / (16.8f - 13.2f)) * 100.0f;
    batData.remaining = (uint8_t)constrain(pct, 0.0f, 100.0f);
  }

  bool isIMUReady() const { return _imuReady; }
  bool isGPSFix() { return _gps.location.isValid(); }
  bool isTCADetected() const { return _tcaDetected; }
};
