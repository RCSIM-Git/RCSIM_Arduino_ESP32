/*
 * Copyright (c) 2026 RCSIM / Mateusz Buzek. All rights reserved.
 * 
 * RCSIM - Konwerter Serial na PPM dla Arduino (v3.0 - High Stability)
 * Ten plik jest częścią projektu RCSIM i jest udostępniony na licencji Open Source.
 * 
 * Licencja: MIT License
 * =================================================================================
 * Wersja: 1.0
 * =================================================================================
 */

#include "PPMEncoder.h" // Biblioteka odpowiedzialna za generowanie sygnału PPM

// --- KONFIGURACJA ---
#define BAUD_RATE           115200  // Prędkość komunikacji USB (musi być zgodna z GCS)
#define PPM_OUTPUT_PIN      10      // Pin, na którym generowany jest sygnał PPM (np. dla portu trenera)
#define NUM_CHANNELS        8       // Liczba kanałów PPM (standardowo 8 dla stabilności)
#define LED_PIN             LED_BUILTIN // Pin diody LED do sygnalizacji pracy
#define FAILSAFE_TIMEOUT_MS 500     // Czas oczekiwania na dane przed aktywacją Failsafe (zwiększono dla stabilności)
#define NEUTRAL_PULSE       1500    // Wartość neutralna dla kanałów (1500us)

// Zmienne globalne
String inputString = "";             // Bufor na przychodzące dane tekstowe
unsigned long lastDataReceivedTime = 0; // Czas odebrania ostatniej poprawnej ramki
bool failsafeActive = true;          // Status mechanizmu bezpieczeństwa

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(BAUD_RATE);           // Rozpoczęcie komunikacji szeregowej
  inputString.reserve(100);          // Rezerwacja pamięci dla bufora (optymalizacja)

  // Na starcie wyłączamy sygnał (bezpieczeństwo - pin jako wejście)
  // Aparatura MT12/RM wykryje brak sygnału jako "Signal Lost"
  pinMode(PPM_OUTPUT_PIN, INPUT);
  ppmEncoder.begin(PPM_OUTPUT_PIN, NUM_CHANNELS, false); // Polaryzacja pozytywna dla większości aparatur

  Serial.println("RCSIM Arduino Bridge (Kill-Switch) Ready.");
}

void loop() {
  // Odczyt danych z portu szeregowego (format: "ch1,ch2,...,ch8\n")
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      if (inputString.length() > 10) { // Podstawowa walidacja długości pakietu
        processInputString();
      }
      inputString = "";
    } else if (inChar != '\r' && inChar != ' ') {
      inputString += inChar;
    }
  }

  // Krytyczny Failsafe - Fizyczne odcięcie sygnału PPM (Signal Kill)
  // Jeśli dane nie dotrą w ciągu FAILSAFE_TIMEOUT_MS, pin przechodzi w tryb INPUT
  if (!failsafeActive && (millis() - lastDataReceivedTime > FAILSAFE_TIMEOUT_MS)) {
    failsafeActive = true;
    pinMode(PPM_OUTPUT_PIN, INPUT); // Fizyczne odcięcie sygnału od aparatury
    digitalWrite(LED_PIN, LOW);
    Serial.println("FAILSAFE: Signal Killed.");
  }
}

/**
 * Funkcja do parsowania odebranego ciągu i ustawiania kanałów PPM.
 * Implementuje atomowe odświeżanie - aktualizacja następuje tylko po odebraniu pełnej ramki.
 */
void processInputString() {
  int values[NUM_CHANNELS];
  int count = 0;
  int currentVal = 0;
  bool parsingNumber = false;

  // Szybkie parsowanie w miejscu (bez alokacji pamięci na nowe obiekty String)
  for (unsigned int i = 0; i < inputString.length(); i++) {
    char c = inputString.charAt(i);
    if (c >= '0' && c <= '9') {
      currentVal = currentVal * 10 + (c - '0');
      parsingNumber = true;
    } else if (c == ',') {
      if (parsingNumber && count < NUM_CHANNELS) {
        // Clamping i sanityzacja: 800-2200us
        if (currentVal < 800 || currentVal > 2200) currentVal = 1500;
        values[count++] = currentVal;
        currentVal = 0;
        parsingNumber = false;
      }
    }
  }

  // Zapisz ostatnią wartość po ostatnim przecinku (lub jego braku)
  if (parsingNumber && count < NUM_CHANNELS) {
    if (currentVal < 800 || currentVal > 2200) currentVal = 1500;
    values[count++] = currentVal;
  }

  // Aktualizujemy kanały TYLKO jeśli odebrano pełną ramkę (zapobiega błędnym ruchom serw)
  if (count == NUM_CHANNELS) {
    if (failsafeActive) {
      failsafeActive = false;
      pinMode(PPM_OUTPUT_PIN, OUTPUT); // Przywracamy sygnał PPM (pin jako wyjście)
      Serial.println("Signal Restored.");
    }
    
    // Przekazanie wartości do biblioteki kodera
    for (int i = 0; i < NUM_CHANNELS; i++) {
      ppmEncoder.setChannel(i, values[i]);
    }
    lastDataReceivedTime = millis();  // Odświeżenie timera aktywności
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Mrugnięcie diodą przy każdym pakiecie
  }
}