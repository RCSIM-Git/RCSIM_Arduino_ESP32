/*
 * =================================================================================
 * RCSIM - USB PC to ESP-NOW CRSF Bridge (Transmitter Dongle)
 * =================================================================================
 * Płytka: Dowolne ESP32 (NodeMCU / ESP32-WROOM-32 / S2 / S3 / C3)
 * Połączenie: Kabel USB z komputerem PC (Stacja Naziemna RCSIM GCS)
 * 
 * Zasada działania:
 *   1. USB RX (PC -> ESP32): Odbiera strumień binarny CRSF (115200 lub 420000 bps)
 *      z aplikacji GCS (ramki 0x16 RC_CHANNELS_PACKED) i natychmiast rozsyła je
 *      przez bezprzewodowy protokół ESP-NOW (Broadcast lub dedykowany MAC pojazdu).
 * 
 *   2. ESP-NOW RX (Pojazd -> ESP32): Odbiera pakiety telemetrii CRSF (0x02 GPS,
 *      0x08 Battery, 0x1E Attitude IMU, 0x14 Link Stats) i natychmiast wypycha je
 *      przez USB TX do komputera PC.
 * 
 * Czas opóźnienia toru radiowego: ~1 - 2 ms!
 * =================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define SERIAL_BAUD_RATE    115200  // Zgodne z profilem CRSF Direct w RCSIM GCS
#define ESPNOW_MAX_PAYLOAD  250

// Domyślny adres Broadcast - odbierze każdy pojazd w zasięgu
// Jeśli chcesz sparować na stałe z jednym odbiornikiem, wpisz jego adres MAC:
// np. uint8_t targetVehicleMac[6] = {0x34, 0x85, 0x18, 0xAA, 0xBB, 0xCC};
uint8_t targetVehicleMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Bufor kołowy dla danych odbieranych z portu szeregowego USB
#define USB_BUF_SIZE 512
uint8_t usbBuf[USB_BUF_SIZE];
uint16_t usbHead = 0;

// Statystyki
volatile uint32_t packetsFromPC = 0;
volatile uint32_t packetsFromAir = 0;
unsigned long lastStatPrint = 0;

// ---------------------------------------------------------------------------------
// Callback odbioru danych radiowych (Pojazd -> PC)
// ---------------------------------------------------------------------------------
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  packetsFromAir++;
  // Natychmiastowe wypchnięcie surowych bajtów CRSF do portu szeregowego USB
  Serial.write(incomingData, len);
}

void setup() {
  // 1. Port szeregowy USB do komunikacji z GCS
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setRxBufferSize(1024); // Zwiększenie bufora wejściowego UART

  delay(200);

  // 2. Inicjalizacja Wi-Fi w trybie Station (bez łączenia z routerem)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // 3. Inicjalizacja protokołu ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("\n[BŁĄD] Inicjalizacja ESP-NOW nie powiodła się!");
    while (true) { delay(1000); }
  }

  // 4. Rejestracja callbacku odbioru telemetrii
  esp_now_register_recv_cb(onDataRecv);

  // 5. Rejestracja peera nadawczego (Pojazd lub Broadcast)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, targetVehicleMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESP-NOW] Ostrzeżenie: Rejestracja peera zakończona kodem błędu.");
  }
}

// ---------------------------------------------------------------------------------
// Pętla główna: USB RX -> ESP-NOW TX (Pętla 100-250 Hz)
// ---------------------------------------------------------------------------------
void loop() {
  // Zbieranie bajtów przychodzących z PC przez USB
  while (Serial.available() > 0) {
    uint8_t b = Serial.read();
    usbBuf[usbHead++] = b;

    // Detekcja końca ramki CRSF lub zapełnienia bufora
    // Standardowa ramka CRSF: [Sync 0xC8/0xEE/0xEA][Len][Type][Payload...][CRC]
    // Całkowita długość ramki = Len + 2 bajty (Sync + Len)
    if (usbHead >= 2) {
      uint8_t expectedTotalLen = usbBuf[1] + 2;
      if (usbHead == expectedTotalLen) {
        // Skompletowano pełną ramkę CRSF - wyślij w powietrze przez ESP-NOW!
        esp_now_send(targetVehicleMac, usbBuf, usbHead);
        packetsFromPC++;
        usbHead = 0;
      } else if (usbHead > expectedTotalLen || usbHead >= ESPNOW_MAX_PAYLOAD) {
        // Błąd synchronizacji lub zbyt duży pakiet - zresetuj bufor
        usbHead = 0;
      }
    }
  }

  // Zabezpieczenie przed wiszącymi bajtami w buforze
  static unsigned long lastByteTime = 0;
  if (usbHead > 0) {
    if (millis() - lastByteTime > 10) {
      // Jeśli przez 10 ms nie doszły brakujące bajty ramki, wyczyść bufor
      usbHead = 0;
    }
    lastByteTime = millis();
  }
}
