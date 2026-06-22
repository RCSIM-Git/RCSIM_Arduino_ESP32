/**
 * Copyright (c) 2026 RCSIM / Mateusz Buzek. All rights reserved.
 *
 * main.cpp - USB-to-PPM Bridge Firmware for Seeed Studio XIAO RP2350 (Tier 1)
 * Platform: PlatformIO / Arduino Framework (Earle Philhower pico core)
 *
 * Architektura dwurdzeniowa:
 *   Rdzeń 0 (Core 0): Odbiór i parsowanie ramek tekstowych CSV z GCS + Failsafe
 *   Rdzeń 1 (Core 1): Generowanie sygnału PPM o wysokiej stabilności
 */

#include <Arduino.h>
#include <hardware/gpio.h>
#include <hardware/sync.h>

// ============================================================================
// Stałe i makra konfiguracyjne
// ============================================================================

#define PPM_CHANNELS       8
#define PPM_FRAME_LENGTH   22500   // Standardowa ramka PPM: 22.5 ms [us]
#define PPM_PULSE_LENGTH   300     // Impuls synchronizujący: 300 us

// Wybór trybu wyjściowego PPM (odkomentowane = Push-Pull, zakomentowane = Open-Drain)
#define PPM_USE_PUSH_PULL

// Nowe, bezpieczne przypisanie pinów 5V-tolerant na płytce Seeed Studio XIAO RP2350
#define PPM_OUT_PIN        2       // Pin D8 (GPIO 2) - Wyjście PPM/CPPM do testu na porcie AUX
#define LED_PIN            LED_BUILTIN

// Timeout watchdoga failsafe [ms]
#define FAILSAFE_TIMEOUT   500

// ============================================================================
// Współdzielone zmienne (Zoptymalizowany podwójny bufor lock-free)
// ============================================================================

volatile uint16_t rc_channels_buffer[2][PPM_CHANNELS] = {
    {1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500},
    {1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500}
};
volatile uint8_t rc_buffer_read_idx = 0; // Wskazuje bufor, z którego Core 1 odczytuje dane

volatile uint32_t last_rx_time = 0;
volatile bool     failsafe_active = true;
volatile bool     estop_active = false;

// ============================================================================
// Funkcje pomocnicze pobierania kanałów (Lock-Free i Failsafe-Safe)
// ============================================================================

static void get_rc_channels(uint16_t *dest) {
    if (failsafe_active || estop_active) {
        // Wszystkie kanały domyślnie na środek (1500 us), oprócz gazu (kanał 3 / indeks 2 -> 1000 us)
        for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
            dest[i] = (i == 2) ? 1000 : 1500;
        }
    } else {
        uint8_t read_idx = rc_buffer_read_idx;
        __dmb(); // Bariera pamięci
        for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
            dest[i] = rc_channels_buffer[read_idx][i];
        }
    }
}

// ============================================================================
// Generator PPM — precyzyjna pętla czasowa na Core 1
// ============================================================================

static void ppm_init_gpio() {
    gpio_init(PPM_OUT_PIN);
#ifdef PPM_USE_PUSH_PULL
    gpio_set_dir(PPM_OUT_PIN, GPIO_OUT);
    gpio_put(PPM_OUT_PIN, true);
    gpio_set_drive_strength(PPM_OUT_PIN, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_slew_rate(PPM_OUT_PIN, GPIO_SLEW_RATE_FAST);
#else
    gpio_put(PPM_OUT_PIN, false);
    gpio_set_dir(PPM_OUT_PIN, GPIO_IN);
    gpio_pull_up(PPM_OUT_PIN);
#endif
}

static void generate_ppm_frame() {
    uint32_t ints = save_and_disable_interrupts();

    uint16_t temp_channels[PPM_CHANNELS];
    get_rc_channels(temp_channels);

    uint32_t accumulated_time = 0;

    for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
        // Impuls LOW (rozpoczęcie kanału)
#ifdef PPM_USE_PUSH_PULL
        gpio_put(PPM_OUT_PIN, false);
#else
        gpio_set_dir(PPM_OUT_PIN, GPIO_OUT);
#endif
        uint32_t pulse_start = time_us_32();
        while (time_us_32() - pulse_start < PPM_PULSE_LENGTH) {
            __compiler_memory_barrier();
        }

        // Stan wysoki (HIGH) - czas trwania kanału
#ifdef PPM_USE_PUSH_PULL
        gpio_put(PPM_OUT_PIN, true);
#else
        gpio_set_dir(PPM_OUT_PIN, GPIO_IN);
#endif
        uint16_t duration = temp_channels[i];
        if (duration < 800) duration = 800;
        if (duration > 2200) duration = 2200;

        uint32_t high_time = duration - PPM_PULSE_LENGTH;
        accumulated_time += duration;

        pulse_start = time_us_32();
        while (time_us_32() - pulse_start < high_time) {
            __compiler_memory_barrier();
        }
    }

    // Impuls synchronizujący (LOW) na końcu ramki
#ifdef PPM_USE_PUSH_PULL
    gpio_put(PPM_OUT_PIN, false);
#else
    gpio_set_dir(PPM_OUT_PIN, GPIO_OUT);
#endif
    uint32_t pulse_start = time_us_32();
    while (time_us_32() - pulse_start < PPM_PULSE_LENGTH) {
        __compiler_memory_barrier();
    }

    // Stan wysoki (HIGH) dla przerwy synchronizacyjnej
#ifdef PPM_USE_PUSH_PULL
    gpio_put(PPM_OUT_PIN, true);
#else
    gpio_set_dir(PPM_OUT_PIN, GPIO_IN);
#endif
    uint32_t sync_time = PPM_FRAME_LENGTH - (accumulated_time + PPM_PULSE_LENGTH);
    if (sync_time < PPM_PULSE_LENGTH) sync_time = PPM_PULSE_LENGTH;

    pulse_start = time_us_32();
    while (time_us_32() - pulse_start < sync_time) {
        __compiler_memory_barrier();
    }

    restore_interrupts(ints);
}

// ============================================================================
// RDZEŃ 0: Obsługa portu USB CDC (Odbiór z GCS i Failsafe)
// ============================================================================

String inputString = "";

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // XIAO RP2350 user LED (active LOW)

    Serial.begin(115200);
    inputString.reserve(128);
    last_rx_time = millis();
}

void processInputString() {
    if (estop_active) {
        return;
    }

    int values[PPM_CHANNELS];
    int count = 0;
    int currentVal = 0;
    bool parsingNumber = false;

    for (unsigned int i = 0; i < inputString.length(); i++) {
        char c = inputString.charAt(i);
        if (c >= '0' && c <= '9') {
            currentVal = currentVal * 10 + (c - '0');
            parsingNumber = true;
        } else if (c == ',') {
            if (parsingNumber && count < PPM_CHANNELS) {
                if (currentVal < 800 || currentVal > 2200) currentVal = 1500;
                values[count++] = currentVal;
                currentVal = 0;
                parsingNumber = false;
            }
        }
    }

    if (parsingNumber && count < PPM_CHANNELS) {
        if (currentVal < 800 || currentVal > 2200) currentVal = 1500;
        values[count++] = currentVal;
    }

    if (count == PPM_CHANNELS) {
        uint8_t write_idx = 1 - rc_buffer_read_idx;
        for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
            rc_channels_buffer[write_idx][i] = values[i];
        }
        __dmb();
        rc_buffer_read_idx = write_idx;

        if (failsafe_active) {
            failsafe_active = false;
            Serial.println("Signal Restored.");
        }
        last_rx_time = millis();
    }
}

void loop() {
    while (Serial.available() > 0) {
        char inChar = (char)Serial.read();
        if (inChar == '\n') {
            inputString.trim();
            if (inputString.equals("ESTOP")) {
                estop_active = true;
                failsafe_active = true;
                Serial.println("E-STOP TRIGGERED: Signal Killed.");
            } else if (inputString.equals("ARM")) {
                if (estop_active) {
                    estop_active = false;
                    Serial.println("E-STOP CLEARED: Ready to Arm.");
                }
            } else if (inputString.length() > 10) {
                processInputString();
            }
            inputString = "";
        } else if (inChar != '\r' && inChar != ' ') {
            inputString += inChar;
        }
    }

    // Failsafe watchdog
    if (!failsafe_active && (millis() - last_rx_time > FAILSAFE_TIMEOUT)) {
        failsafe_active = true;
        Serial.println("FAILSAFE: Signal Killed.");
    }

    // LED status indicator (active LOW)
    if (failsafe_active || estop_active) {
        // Szybkie miganie przy awarii / braku komunikacji
        digitalWrite(LED_PIN, (millis() % 200 < 100) ? LOW : HIGH);
    } else {
        // Wolne miganie przy aktywnej komunikacji
        digitalWrite(LED_PIN, (millis() % 2000 < 1000) ? LOW : HIGH);
    }
}

// ============================================================================
// RDZEŃ 1: Wytwarzanie sygnału PPM
// ============================================================================

void setup1() {
    ppm_init_gpio();
}

void loop1() {
    if (failsafe_active || estop_active) {
        // Wymuszamy stan spoczynkowy (HIGH) na pinie wyjściowym w failsafe
#ifdef PPM_USE_PUSH_PULL
        gpio_put(PPM_OUT_PIN, true);
#else
        gpio_set_dir(PPM_OUT_PIN, GPIO_IN);
#endif
        sleep_ms(2);
        return;
    }

    generate_ppm_frame();
}
