/*
 * =================================================================================
 * RCSIM - Sprzętowy Watchdog i Muxer dla ESP32 (RCSIM HAT V1.0)
 * =================================================================================
 * Wersja: 1.0 (Stabilna)
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

// --- KONFIGURACJA PINÓW ---
#define PIN_HEARTBEAT      4
#define PIN_OVR_STATUS     5
#define PIN_SBUS_RX        15

// --- PARAMETRY WATCHDOGA ---
#define HEARTBEAT_TIMEOUT  1000  // ms (jeśli brak impulsu przez 1s -> Failsafe)
#define NEUTRAL_PWM        307   // ~1500us @ 50Hz (12-bit: 0-4095)
#define NUM_CHANNELS       16

// --- OBIEKTY ---
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

// --- ZMIENNE GLOBALNE ---
volatile unsigned long lastHeartbeatTime = 0;
bool rpi_alive = false;
bool manual_override = false;
unsigned long lastSbusTime = 0;

// Próg kanału aparatury przełączający na Manual (np. Kanał 5 > 1500)
const int OVR_CHANNEL_IDX = 4; // Kanał 5 (indeks 4)
const int OVR_THRESHOLD = 1600;

// Struktura dla wartości kanałów (uproszczona obsługa SBUS)
int channels[NUM_CHANNELS];

// Przerwanie dla Heartbeat (RPi przełącza stan pinu)
void IRAM_ATTR handleHeartbeat() {
    lastHeartbeatTime = millis();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- RCSIM ESP32 Watchdog Init ---");

    // Konfiguracja pinów
    pinMode(PIN_HEARTBEAT, INPUT_PULLUP);
    pinMode(PIN_OVR_STATUS, OUTPUT);
    digitalWrite(PIN_OVR_STATUS, HIGH); // HIGH = Normal, LOW = Override

    // I2C i PCA9685
    Wire.begin(21, 22);
    pca.begin();
    pca.setPWMFreq(50);

    // Przerwanie na Heartbeat
    attachInterrupt(digitalPinToInterrupt(PIN_HEARTBEAT), handleHeartbeat, CHANGE);

    // Inicjalizacja kanałów wartością neutralną
    for(int i=0; i<NUM_CHANNELS; i++) channels[i] = 1500;

    Serial.println("Watchdog aktywny. Oczekiwanie na sygnał Heartbeat...");
}

void loop() {
    unsigned long now = millis();

    // 1. Sprawdzenie Heartbeat od RPi
    if (now - lastHeartbeatTime < HEARTBEAT_TIMEOUT) {
        if (!rpi_alive) {
            Serial.println("RPi wykryte (Heartbeat OK).");
            rpi_alive = true;
        }
    } else {
        if (rpi_alive) {
            Serial.println("ALARM: Brak Heartbeat! RPi zawieszone.");
            rpi_alive = false;
        }
    }

    // 2. Obsługa sygnału SBUS (Placeholder - wymaga biblioteki SBUS w środowisku Arduino)
    // Symulacja odczytu: Jeśli używasz biblioteki BolderFlight SBUS:
    /*
    if (sbus.read(&channels[0], &failsafe, &lostFrame)) {
        lastSbusTime = now;
        if (channels[OVR_CHANNEL_IDX] > OVR_THRESHOLD) {
            manual_override = true;
        } else {
            manual_override = false;
        }
    }
    */
    
    // Na potrzeby szkieletu zakładamy, że manual_override jest sterowany logicznie
    // (W prawdziwym wdrożeniu tu następuje odczyt Serial2 na PIN_SBUS_RX)

    // 3. Logika Wykonawcza (Muxer)
    if (!rpi_alive || manual_override) {
        // AKCJA: Przejęcie kontroli przez ESP32
        digitalWrite(PIN_OVR_STATUS, LOW); // Informuj RPi o przejęciu (jeśli żyje)
        
        if (!rpi_alive) {
            // RPi nie żyje -> Tryb Failsafe (Neutral)
            applySafeState();
        } else {
            // Manual Override -> Przekazuj sygnały z aparatury do PCA9685
            applyManualRC();
        }
    } else {
        // Tryb Normalny: RPi steruje PCA9685, ESP32 jest w trybie czuwania
        digitalWrite(PIN_OVR_STATUS, HIGH);
    }

    delay(20); // Pętla ~50Hz
}

void applySafeState() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        pca.setPWM(i, 0, NEUTRAL_PWM);
    }
}

void applyManualRC() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        // Mapowanie mikrosekund (1000-2000) na PWM PCA9685 (205-410)
        int pwm = map(channels[i], 1000, 2000, 205, 410);
        pca.setPWM(i, 0, pwm);
    }
}
