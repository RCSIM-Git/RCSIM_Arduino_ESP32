/*
 * =================================================================================
 * RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub)
 * =================================================================================
 * Pełna implementacja zaawansowanego huba sterowania dla ESP32 z:
 *   - Natywnym protokołem CRSF (Crossfire / ELRS) i sumą CRC8 DVB-S2
 *   - Wielowarstwową transmisją danych: ESP-NOW (Ultra Low-Latency) / UART / UDP
 *   - Sterownikiem serw/ESC PCA9685 (I2C Fast Mode 400kHz + Fail-Safe)
 *   - Odczytem IMU (MPU6050/MPU9250 z DLPF), GPS (NMEA TinyGPS++) oraz ADC1 Baterii
 *   - Architekturą FreeRTOS Dual-Core (Izolacja radia od serw) + Task WDT
 * =================================================================================
 */

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "CRSFParser.h"
#include "CRSFTransport.h"
#include "PCA9685Manager.h"
#include "SensorsManager.h"

// --- KONFIGURACJA SPRZĘTOWA I PINY ---
#define I2C_SDA_PIN          13
#define I2C_SCL_PIN          14
#define PCA_CALIB_CH         15
#define PCA_CALIB_PIN        12

#define GPS_RX_PIN           32
#define GPS_TX_PIN           -1
#define GPS_BAUDRATE         9600

#define VBAT_ADC_PIN         33
#define VBAT_DIVIDER_RATIO   5.545f // (10k + 2.2k) / 2.2k

#define WDT_TIMEOUT_SECONDS  2
#define FAILSAFE_TIMEOUT_MS  150    // 150 ms braku ramki -> natychmiastowy stop

// --- WYBÓR WARSTWY TRANSMISYJNEJ ---
// Dostępne opcje:
//   - TRANSPORT_ESP_NOW        (Ultra Low-Latency 1-2ms, p2p warstwa MAC, brak routera)
//   - TRANSPORT_SERIAL         (USB CDC / UART do LoRa SX1262/SX1280)
//   - TRANSPORT_UDP            (Lokalne Wi-Fi UDP)
//   - TRANSPORT_MICROLINK_VPN  (Tailscale WireGuard VPN przez Internet / 4G LTE)
#define ACTIVE_TRANSPORT     TRANSPORT_ESP_NOW

// Konfiguracja dla trybu MicroLink VPN (Tailscale)
#define TAILSCALE_AUTH_KEY   "tskey-auth-YOUR_AUTH_KEY_HERE"
#define GCS_TAILSCALE_IP     IPAddress(100, 64, 0, 1) // Wirtualne IP stacji GCS w sieci Tailscale

// Obiekty systemowe
HardwareSerial GpsSerial(1);
CRSFParser crsfParser;
PCA9685Manager pcaManager(I2C_SDA_PIN, I2C_SCL_PIN, PCA_CALIB_CH, PCA_CALIB_PIN);
SensorsManager sensors(GpsSerial, VBAT_ADC_PIN, VBAT_DIVIDER_RATIO);

#if (ACTIVE_TRANSPORT == TRANSPORT_ESP_NOW)
  ESPNowTransport transport;
#elif (ACTIVE_TRANSPORT == TRANSPORT_SERIAL)
  SerialTransport transport(Serial, 115200);
#elif (ACTIVE_TRANSPORT == TRANSPORT_UDP)
  UDPTransport transport(12345, 12347);
#elif (ACTIVE_TRANSPORT == TRANSPORT_MICROLINK_VPN)
  MicroLinkVPNTransport transport(TAILSCALE_AUTH_KEY, GCS_TAILSCALE_IP, 12345, 12347);
#endif

// Kolejka kanałów RC przekazywana między rdzeniami
struct RCChannelsMsg {
  uint16_t pulseUs[16];
  bool isArmed;
  bool failsafe;
};

QueueHandle_t rcQueue;
TaskHandle_t commTaskHandle = NULL;

// =================================================================================
// CORE 0: PROTOKÓŁ, ODBIÓR RADIOWY I NADAJNIK TELEMETRII
// =================================================================================
void commTask(void *pvParameters) {
  uint8_t outTelemetryBuf[64];
  unsigned long lastTelemFast = 0;
  unsigned long lastTelemSlow = 0;

  Serial.println("[Core 0] Wątek komunikacji radiowej uruchomiony.");

  while (true) {
    // 1. Odbiór danych ze strumienia transportowego
    while (transport.available() > 0) {
      uint8_t b = transport.read();
      uint8_t frameType = crsfParser.processByte(b);
      if (frameType == CRSF_FRAMETYPE_RC_CHANNELS) {
        // Nowa poprawna ramka kanałów
        RCChannelsMsg msg;
        msg.failsafe = false;
        msg.isArmed = crsfParser.isArmed;
        for (int ch = 0; ch < 16; ch++) {
          msg.pulseUs[ch] = CRSFParser::crsfToUs(crsfParser.channels[ch]);
        }
        xQueueOverwrite(rcQueue, &msg);
      }
    }

    // 2. Sprawdzenie Failsafe komunikacji (Brak ramki > FAILSAFE_TIMEOUT_MS)
    if (millis() - crsfParser.lastValidChannelsTime > FAILSAFE_TIMEOUT_MS) {
      RCChannelsMsg msg;
      msg.failsafe = true;
      msg.isArmed = false;
      for (int ch = 0; ch < 16; ch++) {
        msg.pulseUs[ch] = 1500; // Pozycja neutralna
      }
      xQueueOverwrite(rcQueue, &msg);
    }

    // 3. Szybka pętla telemetrii (Attitude IMU: 50 Hz / 20 ms)
    if (millis() - lastTelemFast >= 20) {
      lastTelemFast = millis();
      CRSF_Attitude_Data att;
      if (sensors.getAttitudeCRSF(att)) {
        uint8_t len = CRSFParser::packAttitude(outTelemetryBuf, att);
        transport.write(outTelemetryBuf, len);
      }
    }

    // 4. Wolna pętla telemetrii (Bateria, Link Stats, GPS: 5 Hz / 200 ms)
    if (millis() - lastTelemSlow >= 200) {
      lastTelemSlow = millis();

      // Bateria (0x08)
      CRSF_Battery_Data bat;
      sensors.getBatteryCRSF(bat);
      uint8_t lenBat = CRSFParser::packBattery(outTelemetryBuf, bat);
      transport.write(outTelemetryBuf, lenBat);

      // Link Statistics (0x14)
      CRSF_LinkStats_Data stats;
      memset(&stats, 0, sizeof(stats));
      stats.uplink_rssi_1 = (uint8_t)abs(transport.getRSSI());
      stats.uplink_link_quality = transport.getLinkQuality();
      uint8_t lenStats = CRSFParser::packLinkStats(outTelemetryBuf, stats);
      transport.write(outTelemetryBuf, lenStats);

      // GPS (0x02) - jeśli złapano Fix
      CRSF_GPS_Data gpsData;
      if (sensors.getGPSCRSF(gpsData)) {
        uint8_t lenGps = CRSFParser::packGPS(outTelemetryBuf, gpsData);
        transport.write(outTelemetryBuf, lenGps);
      }
    }

    transport.loop();
    vTaskDelay(pdMS_TO_TICKS(2)); // Oddanie czasu dla procesora
  }
}

// =================================================================================
// CORE 1: INICJALIZACJA (SETUP)
// =================================================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n===============================================");
  Serial.println("   RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link)  ");
  Serial.println("===============================================");

  // 1. Task Watchdog Timer
  esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
  esp_task_wdt_add(NULL);

  // 2. Kolejka FreeRTOS
  rcQueue = xQueueCreate(1, sizeof(RCChannelsMsg));

  // 3. Magistrala PCA9685
  if (!pcaManager.begin(false)) {
    Serial.println("[BŁĄD KRYTYCZNY] PCA9685 nie odpowiada!");
  }

  // 4. Czujniki (IMU, GPS, ADC)
  sensors.begin(GPS_BAUDRATE, GPS_RX_PIN, GPS_TX_PIN);

  // 5. Warstwa radiowa
  if (!transport.begin()) {
    Serial.println("[BŁĄD KRYTYCZNY] Warstwa transportowa nie wystartowała!");
  }

  // 6. Uruchomienie wątku Core 0
  xTaskCreatePinnedToCore(
    commTask,
    "CRSF_CommTask",
    4096,
    NULL,
    2, // Wysoki priorytet
    &commTaskHandle,
    0  // Core 0
  );

  Serial.println("[Core 1] Setup zakończony pomyślnie. Pętla serw gotowa.");
}

// =================================================================================
// CORE 1: PĘTLA GŁÓWNA - GENEROWANIE PWM I SENSORYKA (LOOP)
// =================================================================================
void loop() {
  esp_task_wdt_reset();
  pcaManager.checkAndRecoverI2C();
  sensors.updateGPS();

  RCChannelsMsg currentRC;
  if (xQueuePeek(rcQueue, &currentRC, 0) == pdTRUE) {
    if (currentRC.failsafe || !currentRC.isArmed) {
      // Model w stanie DISARMED lub FAILSAFE:
      // Wymuś neutralny sygnał (1500 us) na gazie i skręcie
      pcaManager.setChannelUs(CH_STEERING, 1500);
      pcaManager.setChannelUs(CH_THROTTLE, 1500);
      // Pozostałe kanały
      for (uint8_t ch = 2; ch < 16; ch++) {
        pcaManager.setChannelUs(ch, currentRC.pulseUs[ch]);
      }
    } else {
      // Model uzbrojony (ARMED): bezporednie sterowanie z aparatury/GCS
      pcaManager.setChannelUs(CH_STEERING, currentRC.pulseUs[CH_STEERING]);
      pcaManager.setChannelUs(CH_THROTTLE, currentRC.pulseUs[CH_THROTTLE]);
      for (uint8_t ch = 2; ch < 16; ch++) {
        pcaManager.setChannelUs(ch, currentRC.pulseUs[ch]);
      }
    }
  } else {
    // Brak danych w kolejce - twardy Failsafe
    pcaManager.triggerFailsafe();
  }

  delay(5); // Pętla odświeżania PCA ~200 Hz
}
