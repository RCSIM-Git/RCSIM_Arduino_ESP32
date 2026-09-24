/*
 * =================================================================================
 * RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub & Dual-Link Hybrid)
 * =================================================================================
 * Pełna implementacja zaawansowanego huba sterowania dla ESP32 z:
 *   - Trybem Dual-Link Hybrid: jednoczesna obsługa lokalnego radia RF (np. RadioMaster
 *     MT12 + ER5C V2 po UART z prędkością 420 000 baud) oraz łączności przez Internet 5G
 *     (Tailscale WireGuard VPN / Wi-Fi UDP z telefonu Redmi 15 5G)
 *   - Inteligentnym arbitrażem (Muxer): priorytet bezpośredniego radia RF z natychmiastowym
 *     i płynnym przejściem na 5G w przypadku utraty zasięgu (lub wybór przełącznikiem AUX)
 *   - Dwukierunkową telemetrią CRSF wysyłaną symultanicznie do aparatury MT12 oraz RCSIM PC GCS
 *   - Sterownikiem serw/ESC PCA9685 (I2C Fast Mode 400kHz + sprzętowy Fail-Safe 1500us)
 *   - Odczytem IMU (MPU6050/MPU9250 z DLPF), GPS (NMEA TinyGPS++) oraz ADC1 Baterii
 *   - Architekturą FreeRTOS Dual-Core (Core 0: Radio/Net/Muxer, Core 1: Serwa/PCA9685)
 * =================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "CRSFParser.h"
#include "CRSFTransport.h"
#include "CRSFDualLinkMuxer.h"
#include "PCA9685Manager.h"
#include "SensorsManager.h"

// =================================================================================
// 1. WYBÓR TRYBU PRACY SYSTEMU
// =================================================================================
#define MODE_DUAL_LINK_HYBRID      0  // [ZALECANY DLA 5G + RF] Jednoczesny Muxer: ER5c UART + 5G/VPN
#define MODE_SINGLE_ESP_NOW        1  // Pojedynczy link ESP-NOW (Ultra Low-Latency 1-2ms, tor)
#define MODE_SINGLE_SERIAL         2  // Pojedynczy link USB Serial / LoRa UART
#define MODE_SINGLE_UDP            3  // Pojedynczy link Wi-Fi UDP
#define MODE_SINGLE_MICROLINK_VPN  4  // Pojedynczy link Tailscale VPN

#define ACTIVE_SYSTEM_MODE         MODE_DUAL_LINK_HYBRID

// =================================================================================
// 2. KONFIGURACJA PINÓW I SPRZĘTU
// =================================================================================

// I2C (PCA9685 + IMU MPU6050/MPU9250)
#define I2C_SDA_PIN          13
#define I2C_SCL_PIN          14
#define PCA_CALIB_CH         15
#define PCA_CALIB_PIN        12

// UART dla GPS (TinyGPS++)
#define GPS_RX_PIN           32
#define GPS_TX_PIN           -1
#define GPS_BAUDRATE         9600

// UART dla Lokalnego Odbiornika ELRS (RadioMaster ER5C V2 / XR4 / Nano RX)
// Standardowy protokół CRSF działa z prędkością 420 000 baud
#define CRSF_RF_RX_PIN       16   // ESP32 RX2 <- Odbiornik TX (CRSF out)
#define CRSF_RF_TX_PIN       17   // ESP32 TX2 -> Odbiornik RX (CRSF telemetria in)
#define CRSF_RF_BAUDRATE     420000

// Dzielnik napięcia baterii (ADC1 - bezpieczny przy aktywnym Wi-Fi)
#define VBAT_ADC_PIN         33
#define VBAT_DIVIDER_RATIO   5.545f // (10k + 2.2k) / 2.2k

// Watchdog i Fail-Safe
#define WDT_TIMEOUT_SECONDS  2
#define FAILSAFE_TIMEOUT_MS  150    // 150 ms braku ramki -> twardy neutral serw i gazu

// Kanały funkcyjne
#define CH_STEERING          0      // CH1: Skręt kół
#define CH_THROTTLE          1      // CH2: Gaz / Hamulec (ESC)
#define CH_ARM_INDEX         4      // CH5 (AUX1): Uzbrojenie modelu (>1350 us = ARM)
#define CH_MUX_SWITCH_INDEX  5      // CH6 (AUX2): Przełącznik Muxera (<1300 RF, 1300-1700 AUTO, >1700 5G)

// =================================================================================
// 3. KONFIGURACJA SIECI (Wi-Fi HOTSPOT / 5G / VPN TAILSCALE)
// =================================================================================
// Dane hotspotu Wi-Fi w telefonie (np. Redmi 15 5G)
const char* WIFI_SSID     = "Redmi_Hotspot";
const char* WIFI_PASSWORD = "rcsim_password";

// Porty UDP dla RCSIM
const uint16_t UDP_LISTEN_PORT   = 12345; // Port nasłuchu na ESP32
const uint16_t GCS_TELEMETRY_PORT = 12347; // Port telemetryczny stacji PC

// Konfiguracja stacji GCS (PC)
IPAddress gcsTargetIP(192, 168, 43, 100);  // Domyślny IP stacji PC lub IP Tailscale (np. 100.64.0.1)
const char* TAILSCALE_AUTH_KEY = "tskey-auth-YOUR_KEY_HERE";

// =================================================================================
// OBIEKTY SYSTEMOWE
// =================================================================================
HardwareSerial GpsSerial(1);
HardwareSerial RfReceiverSerial(2);

PCA9685Manager pcaManager(I2C_SDA_PIN, I2C_SCL_PIN, PCA_CALIB_CH, PCA_CALIB_PIN);
SensorsManager sensors(GpsSerial, VBAT_ADC_PIN, VBAT_DIVIDER_RATIO);

// Obiekty transportowe
#if (ACTIVE_SYSTEM_MODE == MODE_DUAL_LINK_HYBRID)
  // W trybie hybrydowym sieć działa przez UDP (połączone z hotspotem 5G)
  UDPTransport netTransport(UDP_LISTEN_PORT, GCS_TELEMETRY_PORT);
  CRSFDualLinkMuxer dualMuxer(RfReceiverSerial, netTransport, FAILSAFE_TIMEOUT_MS, CH_MUX_SWITCH_INDEX);

#elif (ACTIVE_SYSTEM_MODE == MODE_SINGLE_ESP_NOW)
  ESPNowTransport singleTransport;
  CRSFParser crsfParser;

#elif (ACTIVE_SYSTEM_MODE == MODE_SINGLE_SERIAL)
  SerialTransport singleTransport(Serial, 115200);
  CRSFParser crsfParser;

#elif (ACTIVE_SYSTEM_MODE == MODE_SINGLE_UDP)
  UDPTransport singleTransport(UDP_LISTEN_PORT, GCS_TELEMETRY_PORT);
  CRSFParser crsfParser;

#elif (ACTIVE_SYSTEM_MODE == MODE_SINGLE_MICROLINK_VPN)
  MicroLinkVPNTransport singleTransport(TAILSCALE_AUTH_KEY, gcsTargetIP, UDP_LISTEN_PORT, GCS_TELEMETRY_PORT);
  CRSFParser crsfParser;
#endif

// Kolejka danych sterowania przekazywana z Core 0 do Core 1
struct RCChannelsMsg {
  uint16_t pulseUs[16];
  bool isArmed;
  bool failsafe;
  uint8_t activeSource; // 0=Brak, 1=RF, 2=Net
};

QueueHandle_t rcQueue;
TaskHandle_t commTaskHandle = NULL;

// =================================================================================
// CORE 0: WĄTEK KOMUNIKACJI RADIOWEJ, MUXERA I TELEMETRII (FreeRTOS)
// =================================================================================
void commTask(void *pvParameters) {
  uint8_t outTelemetryBuf[64];
  unsigned long lastTelemFast = 0;
  unsigned long lastTelemSlow = 0;
  unsigned long lastStatsLog  = 0;

  Serial.println("[Core 0] Wątek komunikacji radiowej i arbitrażu uruchomiony.");

  while (true) {
#if (ACTIVE_SYSTEM_MODE == MODE_DUAL_LINK_HYBRID)
    // -----------------------------------------------------------------------------
    // TRYB DUAL-LINK HYBRID (Muxer: RadioMaster ER5C V2 + 5G/VPN)
    // -----------------------------------------------------------------------------
    dualMuxer.update();

    RCChannelsMsg msg;
    LinkSource activeSrc;
    dualMuxer.getControlData(msg.pulseUs, msg.isArmed, msg.failsafe, activeSrc);
    msg.activeSource = (uint8_t)activeSrc;
    xQueueOverwrite(rcQueue, &msg);

    // Szybka telemetria (Attitude IMU: 50 Hz / 20 ms)
    if (millis() - lastTelemFast >= 20) {
      lastTelemFast = millis();
      CRSF_Attitude_Data att;
      if (sensors.getAttitudeCRSF(att)) {
        uint8_t len = CRSFParser::packAttitude(outTelemetryBuf, att);
        dualMuxer.sendTelemetry(outTelemetryBuf, len);
      }
    }

    // Wolna telemetria (Bateria, Link Stats, GPS: 5 Hz / 200 ms)
    if (millis() - lastTelemSlow >= 200) {
      lastTelemSlow = millis();

      // 1. Bateria (0x08)
      CRSF_Battery_Data bat;
      sensors.getBatteryCRSF(bat);
      uint8_t lenBat = CRSFParser::packBattery(outTelemetryBuf, bat);
      dualMuxer.sendTelemetry(outTelemetryBuf, lenBat);

      // 2. Link Statistics (0x14)
      CRSF_LinkStats_Data stats;
      memset(&stats, 0, sizeof(stats));
      stats.uplink_rssi_1 = (activeSrc == LINK_SOURCE_RF) ? 55 : (uint8_t)abs(WiFi.RSSI());
      stats.uplink_link_quality = (activeSrc != LINK_SOURCE_NONE) ? 100 : 0;
      stats.active_antenna = (uint8_t)activeSrc; // 1=RF, 2=5G
      uint8_t lenStats = CRSFParser::packLinkStats(outTelemetryBuf, stats);
      dualMuxer.sendTelemetry(outTelemetryBuf, lenStats);

      // 3. GPS (0x02)
      CRSF_GPS_Data gpsData;
      if (sensors.getGPSCRSF(gpsData)) {
        uint8_t lenGps = CRSFParser::packGPS(outTelemetryBuf, gpsData);
        dualMuxer.sendTelemetry(outTelemetryBuf, lenGps);
      }
    }

    // Logi diagnostyczne Muxera co 3 sekundy
    if (millis() - lastStatsLog >= 3000) {
      lastStatsLog = millis();
      DualLinkStats dStats;
      dualMuxer.getStats(dStats);
      const char *srcStr = (dStats.activeSource == LINK_SOURCE_RF) ? "LOKALNY RF (ER5C/MT12)" :
                           (dStats.activeSource == LINK_SOURCE_NET) ? "INTERNET 5G (PC/VPN)" : "BRAK (FAILSAFE)";
      Serial.printf("[DualMuxer] Aktywny link: %s | RF pkts: %u | Net pkts: %u | Przełączeń: %u\n",
                    srcStr, dStats.totalRfPackets, dStats.totalNetPackets, dStats.switchCount);
    }

#else
    // -----------------------------------------------------------------------------
    // TRYBY POJEDYNCZE (ESP-NOW / Serial / UDP / VPN)
    // -----------------------------------------------------------------------------
    while (singleTransport.available() > 0) {
      uint8_t b = singleTransport.read();
      uint8_t frameType = crsfParser.processByte(b);
      if (frameType == CRSF_FRAMETYPE_RC_CHANNELS) {
        RCChannelsMsg msg;
        msg.failsafe = false;
        msg.isArmed = crsfParser.isArmed;
        msg.activeSource = 1;
        for (int ch = 0; ch < 16; ch++) {
          msg.pulseUs[ch] = CRSFParser::crsfToUs(crsfParser.channels[ch]);
        }
        xQueueOverwrite(rcQueue, &msg);
      }
    }

    if (millis() - crsfParser.lastValidChannelsTime > FAILSAFE_TIMEOUT_MS) {
      RCChannelsMsg msg;
      msg.failsafe = true;
      msg.isArmed = false;
      msg.activeSource = 0;
      for (int ch = 0; ch < 16; ch++) {
        msg.pulseUs[ch] = 1500;
      }
      xQueueOverwrite(rcQueue, &msg);
    }

    if (millis() - lastTelemFast >= 20) {
      lastTelemFast = millis();
      CRSF_Attitude_Data att;
      if (sensors.getAttitudeCRSF(att)) {
        uint8_t len = CRSFParser::packAttitude(outTelemetryBuf, att);
        singleTransport.write(outTelemetryBuf, len);
      }
    }

    if (millis() - lastTelemSlow >= 200) {
      lastTelemSlow = millis();
      CRSF_Battery_Data bat;
      sensors.getBatteryCRSF(bat);
      uint8_t lenBat = CRSFParser::packBattery(outTelemetryBuf, bat);
      singleTransport.write(outTelemetryBuf, lenBat);

      CRSF_LinkStats_Data stats;
      memset(&stats, 0, sizeof(stats));
      stats.uplink_rssi_1 = (uint8_t)abs(singleTransport.getRSSI());
      stats.uplink_link_quality = singleTransport.getLinkQuality();
      uint8_t lenStats = CRSFParser::packLinkStats(outTelemetryBuf, stats);
      singleTransport.write(outTelemetryBuf, lenStats);

      CRSF_GPS_Data gpsData;
      if (sensors.getGPSCRSF(gpsData)) {
        uint8_t lenGps = CRSFParser::packGPS(outTelemetryBuf, gpsData);
        singleTransport.write(outTelemetryBuf, lenGps);
      }
    }

    singleTransport.loop();
#endif

    vTaskDelay(pdMS_TO_TICKS(2)); // Oddanie kwantu czasu dla FreeRTOS
  }
}

// =================================================================================
// CORE 1: INICJALIZACJA SYSTEMU (SETUP)
// =================================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n===========================================================");
  Serial.println("   RCSIM - ESP32 Tier 2 Pro (CRSF Multi-Link Hub & Dual-Mux)   ");
  Serial.println("===========================================================");

  // 1. Inicjalizacja sprzętowego Task Watchdog Timer (2s)
  esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
  esp_task_wdt_add(NULL);

  // 2. Kolejka FreeRTOS
  rcQueue = xQueueCreate(1, sizeof(RCChannelsMsg));

  // 3. Sterownik serw i regulatora PCA9685 (I2C Fast Mode 400kHz)
  if (!pcaManager.begin(false)) {
    Serial.println("[BŁĄD KRYTYCZNY] PCA9685 nie odpowiada! Sprawdź zasilanie i linie I2C.");
  }

  // 4. Inicjalizacja czujników (IMU, GPS, ADC1 Baterii)
  sensors.begin(GPS_BAUDRATE, GPS_RX_PIN, GPS_TX_PIN);

  // 5. Inicjalizacja Wi-Fi (nieblokująca!) dla trybów sieciowych / hotspotu 5G
#if (ACTIVE_SYSTEM_MODE == MODE_DUAL_LINK_HYBRID || ACTIVE_SYSTEM_MODE == MODE_SINGLE_UDP || ACTIVE_SYSTEM_MODE == MODE_SINGLE_MICROLINK_VPN)
  Serial.printf("[Wi-Fi] Rozpoczynanie łączenia z hotspotem '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Nie blokujemy pętli setup() - radio RF ruszy od razu, a Wi-Fi połączy się w tle!
#endif

  // 6. Inicjalizacja wybranej warstwy komunikacji
#if (ACTIVE_SYSTEM_MODE == MODE_DUAL_LINK_HYBRID)
  if (!dualMuxer.begin(CRSF_RF_BAUDRATE, CRSF_RF_RX_PIN, CRSF_RF_TX_PIN)) {
    Serial.println("[BŁĄD] Inicjalizacja DualMuxer nie powiodła się!");
  }
#else
  if (!singleTransport.begin()) {
    Serial.println("[BŁĄD KRYTYCZNY] Warstwa transportowa nie wystartowała!");
  }
#endif

  // 7. Utworzenie zadania komunikacyjnego na rdzeniu Core 0
  xTaskCreatePinnedToCore(
    commTask,
    "CRSF_CommTask",
    4096,
    NULL,
    2, // Wysoki priorytet
    &commTaskHandle,
    0  // Core 0
  );

  Serial.println("[Core 1] Inicjalizacja zakończona sukcesem. Pętla serw gotowa.");
}

// =================================================================================
// CORE 1: PĘTLA GŁÓWNA - GENEROWANIE PWM I FAIL-SAFE (LOOP)
// =================================================================================
void loop() {
  esp_task_wdt_reset();
  pcaManager.checkAndRecoverI2C();
  sensors.updateGPS();

  RCChannelsMsg currentRC;
  if (xQueuePeek(rcQueue, &currentRC, 0) == pdTRUE) {
    if (currentRC.failsafe || !currentRC.isArmed) {
      // Model w stanie DISARMED lub FAILSAFE:
      // Gaz (CH2) i Skręt (CH1) wymuszone na bezpieczną pozycję neutralną (1500 us)
      pcaManager.setChannelUs(CH_STEERING, 1500);
      pcaManager.setChannelUs(CH_THROTTLE, 1500);

      // Pozostałe kanały (np. światła, przełączniki)
      for (uint8_t ch = 2; ch < 16; ch++) {
        pcaManager.setChannelUs(ch, currentRC.pulseUs[ch]);
      }
    } else {
      // Model UZBROJONY (ARMED): bezpośrednie precyzyjne sterowanie z aktywnego źródła
      pcaManager.setChannelUs(CH_STEERING, currentRC.pulseUs[CH_STEERING]);
      pcaManager.setChannelUs(CH_THROTTLE, currentRC.pulseUs[CH_THROTTLE]);

      for (uint8_t ch = 2; ch < 16; ch++) {
        pcaManager.setChannelUs(ch, currentRC.pulseUs[ch]);
      }
    }
  } else {
    // Brak danych w kolejce - twardy awaryjny Fail-Safe
    pcaManager.triggerFailsafe();
  }

  delay(5); // Odświeżanie serw i regulatora z częstotliwością ~200 Hz
}
