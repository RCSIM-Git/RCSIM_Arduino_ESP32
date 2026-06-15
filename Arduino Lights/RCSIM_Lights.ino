/*
 * Copyright (c) 2026 RCSIM / Mateusz Buzek. All rights reserved.
 * 
 * RCSIM - Inteligentny Sterownik Oświetlenia RC dla Arduino Nano (v1.0)
 * Ten plik jest częścią projektu RCSIM i jest udostępniony na licencji Open Source.
 * 
 * Wspiera dwa tryby pracy (autodetekcja):
 * 1. Tryb Pełny (Wielokanałowy): Odczyt sygnałów z 3 kanałów odbiornika PWM (Skręt, Gaz, Aux).
 * 2. Tryb Jednokanałowy (Muxed): Gdy podłączony jest tylko kanał 3 (CH3). 
 *    CH3 steruje kierunkowskazami (1000us=lewy, 2000us=prawy, 1500us=brak), a reszta świateł świeci stale.
 * 
 * Bezpieczeństwo napięciowe: Sterowanie masą (Active-Low) z przełączaniem w tryb wysokiej 
 * impedancji (INPUT) przy wyłączeniu diod (ochrona linii 3.3V przed prądem wstecznym).
 * 
 * Licencja: MIT License
 */

#include <Arduino.h>

// =================================================================================
// --- KONFIGURACJA PINÓW ---
// =================================================================================

// Cyfrowe wyjścia sterujące oświetleniem (Active-Low zgodnie ze schematem)
#define PIN_HEADLIGHT       2  // Reflektory przednie (Żółty przewód JST)
#define PIN_BRAKE           3  // Światła STOP / pozycyjne tył (Czerwony przewód JST)
#define PIN_REVERSE         4  // Światła cofania (Pomarańczowy przewód JST)
#define PIN_LEFT_TURN       5  // Kierunkowskaz lewy (Czerwony przez DPST)
#define PIN_RIGHT_TURN      6  // Kierunkowskaz prawy (Czarny przez DPST)
#define PIN_AUX             7  // Światła dodatkowe / dachowe (Żółty przez SPST)

// Cyfrowe wejścia sygnałów PWM z odbiornika (PORTB)
#define PIN_INPUT_CH1       8  // Wejście kanału 1: Skręt / Kierownica (PB0)
#define PIN_INPUT_CH2       9  // Wejście kanału 2: Gaz / Przepustnica (PB1)
#define PIN_INPUT_CH3       10 // Wejście kanału 3: Przełącznik pomocniczy (PB2)

// =================================================================================
// --- PARAMETRY PRACY ---
// =================================================================================
#define DEBUG_MODE          1  // Ustaw na 1, aby wysyłać odczyty PWM na Serial Monitor (115200 baud)
#define FAILSAFE_TIMEOUT    500 // Czas w [ms], po którym utrata sygnału aktywuje światła awaryjne
#define BLINK_INTERVAL_MS   350 // Częstotliwość migania kierunkowskazów w [ms]

// Progi detekcji PWM [µs]
#define PWM_MIN             1000
#define PWM_NEUTRAL         1500
#define PWM_MAX             2000
#define PWM_DEADBAND        80   // Martwa strefa wokół neutralu

// Zmienne ulotne (volatile) aktualizowane bezpośrednio w przerwaniu PCINT
volatile unsigned long ch1_pulse_start = 0;
volatile unsigned long ch2_pulse_start = 0;
volatile unsigned long ch3_pulse_start = 0;

volatile int ch1_raw_pulse = 1500;
volatile int ch2_raw_pulse = 1500;
volatile int ch3_raw_pulse = 1500;

volatile unsigned long last_ch1_pulse_time = 0;
volatile unsigned long last_ch2_pulse_time = 0;
volatile unsigned long last_ch3_pulse_time = 0;

// Filtrowane wartości kanałów używane w pętli głównej
int ch1_val = 1500;
int ch2_val = 1500;
int ch3_val = 1500;

// Filtry programowe w celu eliminacji szumów (Software Exponential Filter)
// (Usunięto float filter_beta = 0.3 na rzecz obliczeń stałoprzecinkowych dla optymalizacji na 8-bit AVR)

// Ustawienia autokalibracji punktu neutralnego przy starcie
int ch1_neutral = 1500;
int ch2_neutral = 1500;

// Zmienne pomocnicze dla kierunkowskazów
unsigned long turn_left_start_time = 0;
unsigned long turn_right_start_time = 0;
const unsigned long turn_signal_delay_threshold = 300; // Opóźnienie przed włączeniem kierunkowskazu [ms]

// =================================================================================
// --- PRZERWANIE SPRZĘTOWE PCINT (PORTB) ---
// =================================================================================
// Obsługuje piny D8 (PB0), D9 (PB1), D10 (PB2) w sposób nieblokujący
ISR(PCINT0_vect) {
  unsigned long now_us = micros();
  static uint8_t last_portb_state = 0;
  uint8_t current_portb_state = PINB; // Odczyt bieżącego stanu całego rejestru PORTB
  
  // Detekcja zmiany stanu na pinie D8 (PB0 - CH1)
  if ((current_portb_state & _BV(PB0)) != (last_portb_state & _BV(PB0))) {
    if (current_portb_state & _BV(PB0)) {
      ch1_pulse_start = now_us;
    } else {
      int pulse_width = now_us - ch1_pulse_start;
      if (pulse_width >= 800 && pulse_width <= 2200) {
        ch1_raw_pulse = pulse_width;
        last_ch1_pulse_time = millis();
      }
    }
  }
  
  // Detekcja zmiany stanu na pinie D9 (PB1 - CH2)
  if ((current_portb_state & _BV(PB1)) != (last_portb_state & _BV(PB1))) {
    if (current_portb_state & _BV(PB1)) {
      ch2_pulse_start = now_us;
    } else {
      int pulse_width = now_us - ch2_pulse_start;
      if (pulse_width >= 800 && pulse_width <= 2200) {
        ch2_raw_pulse = pulse_width;
        last_ch2_pulse_time = millis();
      }
    }
  }
  
  // Detekcja zmiany stanu na pinie D10 (PB2 - CH3)
  if ((current_portb_state & _BV(PB2)) != (last_portb_state & _BV(PB2))) {
    if (current_portb_state & _BV(PB2)) {
      ch3_pulse_start = now_us;
    } else {
      int pulse_width = now_us - ch3_pulse_start;
      if (pulse_width >= 800 && pulse_width <= 2200) {
        ch3_raw_pulse = pulse_width;
        last_ch3_pulse_time = millis();
      }
    }
  }
  
  last_portb_state = current_portb_state;
}

// =================================================================================
// --- BEZPIECZNE STEROWANIE WYJŚCIEM (ACTIVE-LOW Z ZABEZPIECZENIEM) ---
// =================================================================================
/**
 * Włącza lub wyłącza światło na danym pinie w trybie Active-Low.
 * Gdy stan ma być włączony (true) -> Pin staje się wyjściem w stanie LOW (ściąga do GND).
 * Gdy stan ma być wyłączony (false) -> Pin staje się wejściem (wysoka impedancja).
 * Zapobiega to przepływowi prądu wstecznego z linii 3.3V do zasilania 5V mikrokontrolera.
 */
void writeLight(int pin, bool state) {
  if (state) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else {
    // Stan wysokiej impedancji (Hi-Z), brak połączenia z masą ani z 5V
    pinMode(pin, INPUT);
  }
}

// =================================================================================
// --- FUNKCJE INICJALIZACYJNE ---
// =================================================================================
void setup() {
  // Konfiguracja pinów wejściowych z wewnętrznymi rezystorami podciągającymi
  pinMode(PIN_INPUT_CH1, INPUT_PULLUP);
  pinMode(PIN_INPUT_CH2, INPUT_PULLUP);
  pinMode(PIN_INPUT_CH3, INPUT_PULLUP);
  
  // Wstępne wyłączenie wszystkich świateł (wejście = wysoka impedancja)
  writeLight(PIN_HEADLIGHT, false);
  writeLight(PIN_BRAKE, false);
  writeLight(PIN_REVERSE, false);
  writeLight(PIN_LEFT_TURN, false);
  writeLight(PIN_RIGHT_TURN, false);
  writeLight(PIN_AUX, false);
  
  if (DEBUG_MODE) {
    Serial.begin(115200);
    Serial.println(F("==============================================="));
    Serial.println(F("  RCSIM Smart Light Controller Starting..."));
    Serial.println(F("==============================================="));
  }
  
  // Włączenie przerwaniowych rejestrów AVR dla PORTB
  cli();                  // Zablokuj przerwania na czas konfiguracji
  PCICR |= _BV(PCIE0);    // Włącz Pin Change Interrupt 0 (obsługuje D8-D13)
  PCMSK0 |= _BV(PCINT0);  // Włącz obsługę przerwania dla pinu PB0 (D8)
  PCMSK0 |= _BV(PCINT1);  // Włącz obsługę przerwania dla pinu PB1 (D9)
  PCMSK0 |= _BV(PCINT2);  // Włącz obsługę przerwania dla pinu PB2 (D10)
  sei();                  // Odblokuj przerwania
  
  // Oczekiwanie na ustabilizowanie sygnału z odbiornika w celu autokalibracji neutralu
  delay(500);
  
  // Pobranie punktu neutralnego (jeśli sygnał jest podłączony)
  noInterrupts();
  int initial_ch1 = ch1_raw_pulse;
  int initial_ch2 = ch2_raw_pulse;
  interrupts();
  
  if (initial_ch1 > 1200 && initial_ch1 < 1800) ch1_neutral = initial_ch1;
  if (initial_ch2 > 1200 && initial_ch2 < 1800) ch2_neutral = initial_ch2;
  
  if (DEBUG_MODE) {
    Serial.print(F("Neutral CH1 (Steering): ")); Serial.println(ch1_neutral);
    Serial.print(F("Neutral CH2 (Throttle): ")); Serial.println(ch2_neutral);
  }
}

// =================================================================================
// --- PĘTLA GŁÓWNA ---
// =================================================================================
void loop() {
  unsigned long now_ms = millis();
  
  // Pobranie danych z sekcji przerwań (atomowo)
  noInterrupts();
  int raw_ch1 = ch1_raw_pulse;
  int raw_ch2 = ch2_raw_pulse;
  int raw_ch3 = ch3_raw_pulse;
  unsigned long t_ch1 = last_ch1_pulse_time;
  unsigned long t_ch2 = last_ch2_pulse_time;
  unsigned long t_ch3 = last_ch3_pulse_time;
  interrupts();
  
  // Sprawdzenie aktywności poszczególnych kanałów (detekcja obecności sygnału)
  bool ch1_active = (now_ms - t_ch1 < FAILSAFE_TIMEOUT);
  bool ch2_active = (now_ms - t_ch2 < FAILSAFE_TIMEOUT);
  bool ch3_active = (now_ms - t_ch3 < FAILSAFE_TIMEOUT);
  
  // Zastosowanie filtra dolnoprzepustowego (wykładniczego) w celu eliminacji szumów
  // Optymalizacja ⚡ Bolt: Zastąpiono powolną emulację float (filter_beta = 0.3)
  // mnożeniem i dzieleniem stałoprzecinkowym. Znacząco oszczędza cykle CPU na układach 8-bitowych (AVR).
  ch1_val = ch1_val + ((raw_ch1 - ch1_val) * 3) / 10;
  ch2_val = ch2_val + ((raw_ch2 - ch2_val) * 3) / 10;
  ch3_val = ch3_val + ((raw_ch3 - ch3_val) * 3) / 10;
  
  // Stan migania kierunkowskazów (generator fali prostokątnej bezblokujący)
  bool blink_state = (now_ms % (2 * BLINK_INTERVAL_MS)) < BLINK_INTERVAL_MS;
  
  // Zmienne wyjściowe dla oświetlenia
  bool headlight_out = false;
  bool brake_out = false;
  bool reverse_out = false;
  bool left_turn_out = false;
  bool right_turn_out = false;
  bool aux_out = false;
  
  // =================================================================================
  // Tryb 1: Failsafe (Brak podłączonego odbiornika lub utrata sygnału ze wszystkich kanałów)
  // =================================================================================
  if (!ch1_active && !ch2_active && !ch3_active) {
    // Utrata sygnału: Wszystkie główne światła wyłączone, kierunkowskazy migają jako awaryjne (Hazard)
    headlight_out = false;
    brake_out = false;
    reverse_out = false;
    left_turn_out = blink_state;
    right_turn_out = blink_state;
    aux_out = false;
    
    if (DEBUG_MODE && (now_ms % 2000 < 5)) {
      Serial.println(F("STATUS: [FAILSAFE] No signal on all channels!"));
    }
  }
  // =================================================================================
  // Tryb 2: Jednokanałowy Fallback (Muxed) - Podłączony tylko CH3 do D10
  // =================================================================================
  else if (ch3_active && !ch1_active && !ch2_active) {
    // Reflektory, światła stopu i pomocnicze świecą cały czas zgodnie z prośbą Użytkownika
    headlight_out = true;
    brake_out = true;
    aux_out = true;
    reverse_out = false;
    
    // Podział kanału CH3 na sygnały sterowania kierunkowskazami:
    // ~1000us (pozycja dolna) -> Lewy kierunkowskaz miga
    // ~2000us (pozycja górna) -> Prawy kierunkowskaz miga
    // ~1500us (pozycja środkowa) -> Brak migania
    if (ch3_val < 1300) {
      left_turn_out = blink_state;
      right_turn_out = false;
    } else if (ch3_val > 1700) {
      left_turn_out = false;
      right_turn_out = blink_state;
    } else {
      left_turn_out = false;
      right_turn_out = false;
    }
    
    if (DEBUG_MODE && (now_ms % 500 < 5)) {
      Serial.print(F("STATUS: [SINGLE-CHANNEL MUX] CH3 PWM: ")); Serial.print(ch3_val);
      Serial.print(F(" | L: ")); Serial.print(left_turn_out);
      Serial.print(F(" R: ")); Serial.println(right_turn_out);
    }
  }
  // =================================================================================
  // Tryb 3: Pełny Wielokanałowy (Wszystkie sygnały aktywne lub gaz/skręt działają)
  // =================================================================================
  else {
    // 1. Obsługa Kanału 3 (CH3 - Aux Switch) -> Reflektory (D2) i Światła dodatkowe (D7)
    // 3-pozycyjny przełącznik na aparaturze:
    // Pozycja dół (<1300us) -> Wszystko wyłączone
    // Pozycja środek (1300-1700us) -> Reflektory ON, Aux OFF
    // Pozycja góra (>1700us) -> Reflektory ON, Aux ON
    if (ch3_active) {
      if (ch3_val < 1300) {
        headlight_out = false;
        aux_out = false;
      } else if (ch3_val >= 1300 && ch3_val <= 1700) {
        headlight_out = true;
        aux_out = false;
      } else {
        headlight_out = true;
        aux_out = true;
      }
    } else {
      // Brak przełącznika CH3: domyślnie reflektory włączone ze względów bezpieczeństwa
      headlight_out = true;
      aux_out = false;
    }
    
    // 2. Obsługa Kanału 2 (CH2 - Gaz/Throttle) -> STOP (D3) i Cofanie (D4)
    if (ch2_active) {
      // Detekcja strefy neutralnej
      bool is_neutral = (ch2_val >= (ch2_neutral - PWM_DEADBAND) && ch2_val <= (ch2_neutral + PWM_DEADBAND));
      
      // Jazda do przodu (gaz powyżej neutralu + martwa strefa)
      bool is_forward = (ch2_val > (ch2_neutral + PWM_DEADBAND));
      
      // Hamowanie lub jazda w tył (gaz poniżej neutralu - martwa strefa)
      bool is_reverse_or_brake = (ch2_val < (ch2_neutral - PWM_DEADBAND));
      
      if (is_reverse_or_brake) {
        brake_out = true;       // Pełny STOP przy hamowaniu
        reverse_out = true;     // Światła cofania włączone w strefie wstecznej
      } else if (is_neutral) {
        // Pozycja neutralna: Lekki STOP (pozycyjne). Jeśli Twoje LEDy tylne są połączone z PWM,
        // możemy kontrolować jasność (tutaj upraszczamy do 100% stop, lub włączonego na stałe)
        brake_out = true;
        reverse_out = false;
      } else {
        // Jazda do przodu: Brak świateł STOP, brak cofania
        brake_out = false;
        reverse_out = false;
      }
    } else {
      // Brak sygnału gazu: światła tylne włączone stale jako pozycyjne
      brake_out = true;
      reverse_out = false;
    }
    
    // 3. Obsługa Kanału 1 (CH1 - Skręt/Steering) -> Kierunkowskazy automatyczne (D5 / D6)
    if (ch1_active) {
      bool steering_left = (ch1_val < (ch1_neutral - 180));  // Wyraźny skręt w lewo
      bool steering_right = (ch1_val > (ch1_neutral + 180)); // Wyraźny skręt w prawo
      
      // Logika opóźnienia kierunkowskazów (zapobiega mignięciom przy szybkim manewrowaniu)
      if (steering_left) {
        if (turn_left_start_time == 0) {
          turn_left_start_time = now_ms;
        }
        if (now_ms - turn_left_start_time > turn_signal_delay_threshold) {
          left_turn_out = blink_state;
        }
        turn_right_start_time = 0; // Reset drugiego kierunku
      } else if (steering_right) {
        if (turn_right_start_time == 0) {
          turn_right_start_time = now_ms;
        }
        if (now_ms - turn_right_start_time > turn_signal_delay_threshold) {
          right_turn_out = blink_state;
        }
        turn_left_start_time = 0; // Reset drugiego kierunku
      } else {
        // Skręt wyprostowany: reset timerów i brak kierunkowskazów
        turn_left_start_time = 0;
        turn_right_start_time = 0;
        left_turn_out = false;
        right_turn_out = false;
      }
    } else {
      left_turn_out = false;
      right_turn_out = false;
    }
    
    if (DEBUG_MODE && (now_ms % 1000 < 5)) {
      Serial.print(F("STATUS: [MULTI-CHANNEL] CH1: ")); Serial.print(ch1_val);
      Serial.print(F(" | CH2: ")); Serial.print(ch2_val);
      Serial.print(F(" | CH3: ")); Serial.println(ch3_val);
    }
  }
  
  // =================================================================================
  // --- ZAPIS STANÓW NA FIZYCZNE PINY ---
  // =================================================================================
  writeLight(PIN_HEADLIGHT, headlight_out);
  writeLight(PIN_BRAKE, brake_out);
  writeLight(PIN_REVERSE, reverse_out);
  writeLight(PIN_LEFT_TURN, left_turn_out);
  writeLight(PIN_RIGHT_TURN, right_turn_out);
  writeLight(PIN_AUX, aux_out);
  
  // Bardzo małe opóźnienie pętli głównej dla stabilności
  delay(10);
}
