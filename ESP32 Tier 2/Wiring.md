Poradnik Połączeń: ESP32 RCSIM Hub
Ten przewodnik pomoże Ci poprawnie podłączyć peryferia (serwa, czujniki) do Twojej płytki ESP32.

1. Zasilanie (Bardzo Ważne!)
ESP32-CAM i WROVER są bardzo czułe na spadki napięcia, szczególnie przy włączonym WiFi i Kamerze.

ESP32: Zasilaj pin 5V stabilnym napięciem 5V (np. z BEC-a lub dobrego zasilacza USB). Unikaj zasilania samej logiki z pinu 3.3V, jeśli używasz kamery.
PCA9685: Posiada dwa wejścia zasilania:
VCC/GND (Piny boczne): Zasilanie logiki (podłącz do 5V i GND z ESP32).
V+ Blue Terminal (Zacisk śrubowy): Zasilanie serwomechanizmów (podłącz pakiet LiPo lub zewnętrzny zasilacz 5-6V). Nie zasilaj serw bezpośrednio z ESP32!
2. Podłączenie I2C (MPU6050 & PCA9685)
Magistrala I2C jest współdzielona. Oba urządzenia (MPU6050 i PCA9685) podłączasz do tych samych pinów ESP32 równolegle.

Profil: AI-Thinker (Standard ESP32-CAM)
Funkcja	Pin Urządzenia	Pin ESP32
SDA	SDA	GPIO 21
SCL	SCL	GPIO 22
GND	GND	GND
VCC	VCC	5V
Profil: ESP32-Wrover-Dev (Freenove / Kit)
Funkcja	Pin Urządzenia	Pin ESP32
SDA	SDA	GPIO 26
SCL	SCL	GPIO 27
GND	GND	GND
VCC	VCC	5V
3. Sterowanie Serwami (PCA9685)
Serwa wpinasz pionowo w 3-pinowe złącza na PCA9685:

Kolejność (od góry): Masa (Czarny/Brązowy) -> Plus (Czerwony) -> Sygnał (Żółty/Biały).
Kanał 0-15: Zgodnie z tym, co ustawisz w GCS (np. Kanał 0 to zazwyczaj Steering, Kanał 1 to Throttle).
4. Kamera
Kamera OV2640 jest podłączona fabrycznie za pomocą taśmy FPC do gniazda na spodzie płytki. Jeśli masz błąd Camera init failed, upewnij się, że taśma jest dociśnięta, a blokada gniazda zamknięta.

5. Podsumowanie Pinów (Cheat Sheet)
AI-Thinker ESP32-CAM
I2C: SDA=21, SCL=22.
Kamera: Zintegrowana (Piny 32, 0, 26, 27, 35, 34, 39, 36, 21, 19, 18, 5, 25, 23, 22).
Wolne piny: GPIO 12, 13, 14, 15 (można użyć do dodatkowych czujników, ale uwaga - niektóre są używane przez kartę SD).
ESP32-WROVER-DEV
I2C: SDA=26, SCL=27.
Kamera: Zintegrowana (XCLK=21).
Wolne piny: Wrover-Dev ma zazwyczaj wyprowadzone znacznie więcej pinów na bocznych listwach goldpin.
TIP

Jeśli używasz ESP32-CAM, odłącz moduł od programatora USB-TTL po wgraniu kodu, jeśli planujesz zasilać go z zewnętrznego BEC-a, aby uniknąć konfliktów zasilania.

