
/*
 * =================================================================================
 * RCSIM - Bezprzewodowy Hub Sterowania dla ESP32 (V3.1)
 * =================================================================================
 * Wersja: 3.1 (Wsparcie dla Statycznego IP, MJPEG Stream i PCA9685)
 *
 * Cel:
 * Ten szkic przekształca ESP32 w zaawansowany hub sterujący, który:
 * 1. Odbiera polecenia sterujące z GCS (PC) przez UDP.
 * 2. Steruje 16 serwomechanizmami przez PCA9685 (I2C).
 * 3. Strumieniuje wideo z kamery ESP32-CAM jako MJPEG (Port 81).
 * 4. Wysyła telemetrię IMU (MPU6050) do GCS.
 *
 * Kompatybilność:
 * - GCS Mode: "ESP32 (WiFi Lite Mode)"
 * - Video Source: "ESP32 (IP Stream)"
 */

// --- GŁÓWNA KONFIGURACJA ---
#define ENABLE_CAMERA true    // Włącz strumień wideo
#define ENABLE_IMU    true    // Włącz wysyłanie danych z IMU

// --- WYBÓR PROFILU SPRZĘTOWEGO ---
// Odkomentuj tylko jeden z poniższych profili:
//#define BOARD_AI_THINKER  // Standardowe ESP32-CAM
#define BOARD_WROVER_DEV  // ESP32-Wrover-Dev
//#define LILYGO_TSIMCAM_S3    // LILYGO_TSIMCAM_S3

// --- DEFINICJE PINÓW DLA PROFILI ---
#ifdef BOARD_AI_THINKER
  // Piny Kamery AI-Thinker
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  // Piny I2C dla AI-Thinker
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
  #define XCLK_FREQ 20000000
#endif

#ifdef BOARD_WROVER_DEV
  // Piny Kamery Wrover-Dev (Freenove / Kit / ESP32-WROVER-E CAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM     21
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       19
  #define Y4_GPIO_NUM       18
  #define Y3_GPIO_NUM        5
  #define Y2_GPIO_NUM        4
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  // Piny I2C dla Wrover-Dev (Przeniesione na 13/14, aby uniknąć konfliktu z SCCB kamery)
  #define I2C_SDA_PIN 13
  #define I2C_SCL_PIN 14
  #define XCLK_FREQ 20000000  // Zwiększono dla lepszego FPS (standard dla ESP32-CAM)
#endif

#ifdef  BOARD_LILYGO_TSIMCAM_S3
  // Piny Kamery LilyGO S3 (z hardware.md)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    18
  #define XCLK_GPIO_NUM     14
  #define SIOD_GPIO_NUM      4
  #define SIOC_GPIO_NUM      5
  #define Y9_GPIO_NUM       15
  #define Y8_GPIO_NUM       16
  #define Y7_GPIO_NUM       17
  #define Y6_GPIO_NUM       12
  #define Y5_GPIO_NUM       10
  #define Y4_GPIO_NUM        8
  #define Y3_GPIO_NUM        9
  #define Y2_GPIO_NUM       11
  #define VSYNC_GPIO_NUM     6
  #define HREF_GPIO_NUM      7
  #define PCLK_GPIO_NUM     13
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 46
  #define XCLK_FREQ 20000000
#endif

// --- KONFIGURACJA SIECI WiFi ---
const char* ssid = "BUZEK_AP";        // <- WPISZ SWOJE SSID
const char* password = "netiabuzek";     // <- WPISZ SWOJE HASŁO

// Konfiguracja STATYCZNEGO ADRESU IP
// (Musi być zgodny z ustawieniami w GCS tab Konfiguracja)
bool useStaticIP = true; 
IPAddress local_IP(192, 168, 31, 111); 
IPAddress gateway(192, 168, 31, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // Opcjonalnie
IPAddress secondaryDNS(8, 8, 4, 4); // Opcjonalnie

// Konfiguracja portów
const int UDP_LISTEN_PORT = 12345;      // Port sterowania (GCS -> ESP32)
const int PC_TELEMETRY_PORT = 12347;    // Port telemetrii (ESP32 -> GCS)
const int VIDEO_SERVER_PORT = 81;        // Port strumienia MJPEG (http://IP:81/stream)

// Parametry Serw / ESC
#define NUM_CHANNELS 16
#define SERVO_FREQUENCY 50
#define FAILSAFE_TIMEOUT_MS 500
#define NEUTRAL_PULSE 1500

// --- BIBLIOTEKI ---
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#if ENABLE_IMU
#include <MPU6050_light.h>
#endif

#if ENABLE_CAMERA
#include "esp_camera.h"
#endif

// --- OBIEKTY ---
WiFiUDP udp;
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();

#if ENABLE_IMU
MPU6050 mpu(Wire);
#endif

#if ENABLE_CAMERA
WiFiServer videoServer(VIDEO_SERVER_PORT);
#endif

unsigned long lastPacketTime = 0;
bool failsafeActive = true;
IPAddress pcIP; 

void triggerFailsafe() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    int pwm_val = map(NEUTRAL_PULSE, 1000, 2000, 205, 410); 
    pca.setPWM(i, 0, pwm_val);
  }
  failsafeActive = true;
}

void i2c_scan() {
  byte error, address;
  int nDevices = 0;
  Serial.println("Skanowanie magistrali I2C...");
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Znaleziono urządzenie I2C pod adresem 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      if (address == 0x40) Serial.print(" (PCA9685)");
      if (address == 0x68) Serial.print(" (MPU6050)");
      Serial.println();
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("Nie znaleziono żadnych urządzeń I2C!\n");
}

#if ENABLE_CAMERA
void videoTask(void *pvParameters) {
  while (true) {
    WiFiClient client = videoServer.available();
    if (client) {
      client.setNoDelay(true);
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
      client.println();
      
      while (client.connected()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
        
        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(1)); // Maksymalna płynność (odświeżanie zależne od prędkości matrycy)
      }
      client.stop();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- RCSIM ESP32 Hub V3.1 STABLE ---");

  // 1. WiFi
  if (useStaticIP) {
    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  }
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi...");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) { delay(500); Serial.print("."); retry++; }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nPołączono! IP: " + WiFi.localIP().toString());
  }

  // 2. I2C Inicjalizacja
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  i2c_scan(); // Zostawiamy skaner do weryfikacji przy starcie

  // 3. PCA9685
  pca.begin();
  pca.setPWMFreq(SERVO_FREQUENCY);
  triggerFailsafe();
  Serial.println("PCA9685 Gotowy.");

  // 4. IMU
  #if ENABLE_IMU
  if (mpu.begin() == 0) {
    Serial.println("IMU Gotowy.");
    mpu.calcOffsets();
  } else {
    Serial.println("Błąd telemetrii IMU!");
  }
  #endif

  // 5. Kamera
  #if ENABLE_CAMERA
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = XCLK_FREQ;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 15; // Niższa wartość = lepsza jakość (skala 0-63)
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  delay(500); // Czas na stabilizację sensora po starcie WiFi i I2C
  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) {
    videoServer.begin();
    // Uruchomienie serwera wideo na osobnym rdzeniu (Core 0), aby nie spowalniać sterowania
    xTaskCreatePinnedToCore(videoTask, "videoTask", 4096, NULL, 1, NULL, 0);
    Serial.println("Kamera OK. Strumień: http://IP:81/stream");
  } else {
    Serial.printf("Błąd kamery: 0x%x\n", err);
  }
  #endif

  udp.begin(UDP_LISTEN_PORT);
}

void loop() {
  // 1. Sterowanie UDP
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buf[255];
    int len = udp.read(buf, 255);
    if (len > 0) {
      buf[len] = 0;
      pcIP = udp.remoteIP();
      int ch = 0;
      char* t = strtok(buf, ",");
      while (t != NULL && ch < NUM_CHANNELS) {
        int us = constrain(atoi(t), 1000, 2000);
        pca.setPWM(ch, 0, map(us, 1000, 2000, 205, 410));
        t = strtok(NULL, ",");
        ch++;
      }
      lastPacketTime = millis();
      if (failsafeActive) {
        failsafeActive = false;
        Serial.println("Link sterowania OK.");
      }
    }
  }

  // 2. Failsafe
  if (!failsafeActive && (millis() - lastPacketTime > FAILSAFE_TIMEOUT_MS)) {
    Serial.println("FAILSAFE!");
    triggerFailsafe();
  }

  // 3. Telemetria IMU
  #if ENABLE_IMU
  static unsigned long lastTelem = 0;
  if (millis() - lastTelem > 50) { 
    lastTelem = millis();
    mpu.update();
    char j[128];
    snprintf(j, sizeof(j), "{\"imu\":{\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}}",
             mpu.getAccX(), mpu.getAccY(), mpu.getAccZ(), mpu.getGyroX(), mpu.getGyroY(), mpu.getGyroZ());
    if (pcIP) {
      udp.beginPacket(pcIP, PC_TELEMETRY_PORT);
      udp.write((uint8_t*)j, strlen(j));
      udp.endPacket();
    }
  }
  #endif
}
