/*
 * =================================================================================
 * RCSIM - Sprzętowy Watchdog i Muxer dla ESP32 (RCSIM HAT V1.0)
 * =================================================================================
 * Wersja: 1.1 (Zoptymalizowana)
 * 
 * Cel:
 * 1. Monitorowanie sygnału Heartbeat z Raspberry Pi 5.
 * 2. Obsługa manualnego przejęcia kontroli (Manual Override) przez SBUS.
 * 3. Zarządzanie stanem Failsafe sterownika PCA9685.
 * 
 * Pinout (Zgodny z rcsim_hat_schematic_draft.md):
 * - GPIO 4:  Wejście Heartbeat (z RPi GPIO 26)
 * - GPIO 5:  Wyjście statusu Override (do RPi GPIO 21)
 * - GPIO 15: Wejście sygnału SBUS (z odbiornika RC)
 * - I2C (21 SDA, 22 SCL): Komunikacja z PCA9685 (Adres 0x40)
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
// Wymaga biblioteki w PlatformIO / Arduino IDE: bolderflight/Bolder Flight Systems SBUS
#include "sbus.h" 

// --- KONFIGURACJA PINÓW ---
#define PIN_HEARTBEAT      4
#define PIN_OVR_STATUS     5
#define PIN_SBUS_RX        15

// --- PARAMETRY ---
#define HEARTBEAT_TIMEOUT  1000  
#define NEUTRAL_PWM        307   
#define NUM_CHANNELS       16

// Próg kanału aparatury przełączający na Manual (Kanał 5 > 1500)
const int OVR_CHANNEL_IDX = 4; 
const int OVR_THRESHOLD = 1600;

// --- OBIEKTY ---
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

// Użycie sprzętowego Serial2 (TX: -1, RX: 15). 
// Biblioteka BolderFlight obsługuje ESP32 i wbudowaną inwersję.
bfs::SbusRx sbus(&Serial2, PIN_SBUS_RX, -1, true); 

// --- ZMIENNE GLOBALNE I STANY ---
volatile unsigned long lastHeartbeatTime = 0;

// Maszyna stanów dla systemu Muxera
enum SystemState {
    STATE_UNKNOWN,
    STATE_RPI_CONTROL,
    STATE_MANUAL_OVR,
    STATE_FAILSAFE
};
SystemState currentState = STATE_UNKNOWN;

// Przerwanie dla Heartbeat
void IRAM_ATTR handleHeartbeat() {
    lastHeartbeatTime = millis();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- RCSIM ESP32 Watchdog & MUX ---");

    pinMode(PIN_HEARTBEAT, INPUT_PULLUP);
    pinMode(PIN_OVR_STATUS, OUTPUT);
    digitalWrite(PIN_OVR_STATUS, HIGH); // Domyślnie RPi ma kontrolę

    // Inicjalizacja magistrali I2C (ESP32 Master)
    Wire.begin(21, 22);
    pca.begin();
    pca.setPWMFreq(50);

    // Inicjalizacja SBUS (na porcie Serial2)
    sbus.Begin();

    attachInterrupt(digitalPinToInterrupt(PIN_HEARTBEAT), handleHeartbeat, CHANGE);
    
    // Wymuszenie stanu bezpiecznego na start
    applySafeState();
}

void loop() {
    unsigned long now = millis();

    // 1. ODCZYT I WALIDACJA HEARTBEAT Z RPI
    bool rpi_alive = (now - lastHeartbeatTime < HEARTBEAT_TIMEOUT);

    // 2. ODCZYT SYGNAŁU SBUS (RC)
    bool rc_alive = false;
    bool manual_override = false;
    
    if (sbus.Read()) {
        // Jeśli odebraliśmy ramkę i nie ma flagi Failsafe z samej aparatury
        if (!sbus.failsafe()) {
            rc_alive = true;
            // Sprawdzenie przełącznika
            if (sbus.ch(OVR_CHANNEL_IDX) > OVR_THRESHOLD) {
                manual_override = true;
            }
        }
    }

    // 3. LOGIKA DECYZYJNA (Wybór pożądanego stanu)
    SystemState desiredState;

    if (manual_override && rc_alive) {
        // Operator fizycznie włączył przełącznik na aparaturze
        desiredState = STATE_MANUAL_OVR;
    } 
    else if (!rpi_alive) {
        // RPi umarło i brak manual_override (lub brak zasięgu RC)
        desiredState = STATE_FAILSAFE;
    } 
    else {
        // RPi żyje i brak manualnego przejęcia
        desiredState = STATE_RPI_CONTROL;
    }

    // 4. WYKONYWANIE ZMIANY STANU (Unikanie blokowania szyny I2C)
    if (desiredState != currentState) {
        Serial.print("ZMIANA STANU SYSTEMU na: ");
        
        if (desiredState == STATE_FAILSAFE) {
            Serial.println("FAILSAFE (RPi Dead / Zatrzymanie I2C)");
            digitalWrite(PIN_OVR_STATUS, LOW); // Informuj układ, że przejęliśmy
            applySafeState();
        } 
        else if (desiredState == STATE_MANUAL_OVR) {
            Serial.println("MANUAL OVERRIDE (Sterowanie z aparatury)");
            digitalWrite(PIN_OVR_STATUS, LOW);
            // Pierwsze uderzenie I2C robimy natychmiast
            applyManualRC();
        } 
        else if (desiredState == STATE_RPI_CONTROL) {
            Serial.println("RPI CONTROL (Oczekiwanie na komendy z RPi)");
            digitalWrite(PIN_OVR_STATUS, HIGH);
        }
        
        currentState = desiredState;
    }

    // Ciągłe odświeżanie serw TYLKO jeśli jesteśmy w trybie manualnym (zmieniające się wychylenia drążków)
    if (currentState == STATE_MANUAL_OVR && sbus.new_data()) {
        applyManualRC();
    }

    delay(10); // Główna pętla może działać szybko, odświeżanie RC i tak dyktuje SBUS (~9ms)
}

void applySafeState() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        pca.setPWM(i, 0, NEUTRAL_PWM);
    }
}

void applyManualRC() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        // Biblioteka BolderFlight zwraca wartości SBUS w zakresie ok. ~172 - 1811 (zależnie od aparatury)
        int us = constrain(sbus.ch(i), 1000, 2000); // Sanityzacja (wartości mikrosekund)
        int pwm = map(us, 1000, 2000, 205, 410);
        pca.setPWM(i, 0, pwm);
    }
}
