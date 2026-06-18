Connection Guide: ESP32 RCSIM Hub
This guide will help you correctly connect peripherals (servos, sensors) to your ESP32 board.

1. Power Supply (Very Important!)
ESP32-CAM and WROVER are highly sensitive to voltage drops, especially when WiFi and the Camera are active.

ESP32: Power the 5V pin with a stable 5V source (e.g., from a BEC or a good USB power supply). Avoid powering logic solely from the 3.3V pin if you are using a camera.
PCA9685: Features two power inputs:
VCC/GND (Side pins): Logic power (connect to 5V and GND from ESP32).
V+ Blue Terminal (Screw terminal): Servo power (connect a LiPo battery or an external 5-6V power supply). Do not power servos directly from the ESP32!

2. I2C Connection (MPU6050 & PCA9685)
The I2C bus is shared. Both devices (MPU6050 and PCA9685) are connected to the same ESP32 pins in parallel.

Profile: AI-Thinker (Standard ESP32-CAM)
Function    Device Pin    ESP32 Pin
SDA         SDA           GPIO 21
SCL         SCL           GPIO 22
GND         GND           GND
VCC         VCC           5V

Profile: ESP32-Wrover-Dev (Freenove / Kit)
Function    Device Pin    ESP32 Pin
SDA         SDA           GPIO 26
SCL         SCL           GPIO 27
GND         GND           GND
VCC         VCC           5V

3. Servo Control (PCA9685)
Plug servos vertically into the 3-pin headers on the PCA9685:

Order (from top): Ground (Black/Brown) -> Plus (Red) -> Signal (Yellow/White).
Channels 0-15: Map according to your GCS configuration (e.g., Channel 0 is usually Steering, Channel 1 is Throttle).

4. Camera
The OV2640 camera is pre-installed via an FPC ribbon cable to the socket on the underside of the board. If you encounter a Camera init failed error, ensure the ribbon cable is seated properly and the socket lock is closed.

5. Pinout Summary (Cheat Sheet)
AI-Thinker ESP32-CAM
I2C: SDA=21, SCL=22.
Camera: Integrated (Pins 32, 0, 26, 27, 35, 34, 39, 36, 21, 19, 18, 5, 25, 23, 22).
Unused pins: GPIO 12, 13, 14, 15 (can be used for additional sensors, but be aware - some are used by the SD card).

ESP32-WROVER-DEV
I2C: SDA=26, SCL=27.
Camera: Integrated (XCLK=21).
Unused pins: The Wrover-Dev typically breaks out many more pins on the side goldpin headers.

TIP
If you are using an ESP32-CAM, disconnect the module from the USB-TTL programmer after flashing the code if you plan to power it from an external BEC to avoid power source conflicts.
