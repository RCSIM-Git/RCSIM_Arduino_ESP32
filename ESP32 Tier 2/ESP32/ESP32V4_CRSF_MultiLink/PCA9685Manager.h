#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// =================================================================================
// RCSIM - PCA9685 I2C Servo Controller & Fail-Safe Manager
// =================================================================================

#define PCA9685_I2C_ADDR       0x40
#define PCA_NUM_CHANNELS       16
#define SERVO_FREQ_HZ          50     // Standardowy PWM dla modelarstwa (50-330 Hz)
#define OSCILLATOR_FREQ        27000000 // Bazowa częstotliwość oscylatora PCA

// Przypisanie kanałów modelu
#define CH_STEERING            0      // Kanał 1: Skręt
#define CH_THROTTLE            1      // Kanał 2: Gaz / Hamulec
#define CH_GEAR                2      // Kanał 3: Skrzynia biegów / Dodatkowy
#define CH_AUX1_ARM            4      // Kanał 5 (indeks 4): Uzbrojenie (ARM)

class PCA9685Manager {
private:
  Adafruit_PWMServoDriver _pca;
  uint8_t _sdaPin;
  uint8_t _sclPin;
  uint8_t _calibPcaChannel;
  int8_t  _calibFeedbackPin;
  bool    _initialized;

public:
  PCA9685Manager(uint8_t sdaPin = 13, uint8_t sclPin = 14, uint8_t calibCh = 15, int8_t calibPin = 12)
    : _pca(PCA9685_I2C_ADDR), _sdaPin(sdaPin), _sclPin(sclPin),
      _calibPcaChannel(calibCh), _calibFeedbackPin(calibPin), _initialized(false) {}

  bool begin(bool enableAutoCalib = false) {
    Wire.begin(_sdaPin, _sclPin);
    Wire.setClock(400000); // Fast Mode 400 kHz
    Wire.setTimeOut(10);   // 10 ms limit oczekiwania chroniący przed zawieszeniem

    // Szybki skan szyny I2C pod adresem 0x40
    Wire.beginTransmission(PCA9685_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
      Serial.println("[PCA9685] BŁĄD: Nie wykryto układu PCA9685 pod adresem 0x40!");
      return false;
    }

    _pca.begin();
    _pca.setOscillatorFrequency(OSCILLATOR_FREQ);
    _pca.setPWMFreq(SERVO_FREQ_HZ);

    if (enableAutoCalib && _calibFeedbackPin >= 0) {
      calibrateOscillator();
    }

    triggerFailsafe();
    _initialized = true;
    Serial.println("[PCA9685] Zainicjalizowany pomyślnie na szynie I2C Fast Mode (400kHz).");
    return true;
  }

  // Wymuszenie stanu bezpiecznego (Neutral 1500us na sterach, 1500us na gazie)
  void triggerFailsafe() {
    for (uint8_t ch = 0; ch < PCA_NUM_CHANNELS; ch++) {
      setChannelUs(ch, 1500);
    }
  }

  // Ustawienie szerokości impulsu w mikrosekundach (us) dla danego kanału
  void setChannelUs(uint8_t channel, uint16_t pulseUs) {
    if (channel >= PCA_NUM_CHANNELS) return;
    if (pulseUs < 800) pulseUs = 800;
    if (pulseUs > 2200) pulseUs = 2200;

    // Przeliczenie mikrosekund na ticks (0-4095) przy SERVO_FREQ_HZ
    // 1 cykl = (1,000,000 / 50) = 20,000 us -> 4096 ticks
    uint16_t pwmTicks = (uint16_t)(((uint32_t)pulseUs * 4096UL) / (1000000UL / SERVO_FREQ_HZ));
    _pca.setPWM(channel, 0, pwmTicks);
  }

  // Odzyskiwanie magistrali I2C (Bus Recovery) w przypadku blokady linii SDA przez zakłócenia EMI
  void checkAndRecoverI2C() {
    Wire.beginTransmission(PCA9685_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
      Serial.println("[I2C] Wykryto kolizję lub blokadę szyny! Resetowanie linii...");
      Wire.end();
      pinMode(_sdaPin, INPUT_PULLUP);
      pinMode(_sclPin, OUTPUT);

      // Generowanie 9 impulsów zegara, by slave zwolnił linię SDA
      for (int i = 0; i < 9; i++) {
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(5);
        digitalWrite(_sclPin, LOW);
        delayMicroseconds(5);
      }

      Wire.begin(_sdaPin, _sclPin);
      Wire.setClock(400000);
      Wire.setTimeOut(10);
      _pca.begin();
      _pca.setPWMFreq(SERVO_FREQ_HZ);
      triggerFailsafe();
    }
  }

  // Pętla sprzężenia zwrotnego do precyzyjnego strojenia zegara wewnętrznego PCA9685
  void calibrateOscillator() {
    pinMode(_calibFeedbackPin, INPUT);
    uint32_t currentOsc = OSCILLATOR_FREQ;

    for (int iter = 0; iter < 5; iter++) {
      uint16_t targetUs = 1500;
      uint16_t pwmTicks = (uint16_t)(((uint32_t)targetUs * 4096UL) / (1000000UL / SERVO_FREQ_HZ));
      _pca.setPWM(_calibPcaChannel, 0, pwmTicks);

      delay(20);
      unsigned long measuredUs = pulseIn(_calibFeedbackPin, HIGH, 50000);
      if (measuredUs == 0) {
        Serial.println("[PCA9685] Autokalibracja: brak sygnału zwrotnego. Pomijanie.");
        break;
      }

      int diff = (int)measuredUs - (int)targetUs;
      if (abs(diff) <= 3) {
        Serial.printf("[PCA9685] Skalibrowano z tolerancją %d us (Osc: %u Hz)\n", diff, currentOsc);
        break;
      }

      currentOsc = (uint32_t)((float)currentOsc * ((float)measuredUs / (float)targetUs));
      _pca.setOscillatorFrequency(currentOsc);
      _pca.setPWMFreq(SERVO_FREQ_HZ);
    }
  }

  bool isInitialized() const { return _initialized; }
};
