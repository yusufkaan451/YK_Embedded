#include <Arduino.h>
#include <HardwareSerial.h>
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp> 

HardwareSerial MySerial(1); // Use UART1
static LGFX lcd;

const int RX_PIN = 27; // Example RX pin, change as needed
const int TX_PIN = 27; // Example TX pin, change as needed
const long BAUD_RATE = 9600;

float lastDistance = -1.0;

void setup() {
    Serial.begin(115200); // Start the Serial communication with the computer
    MySerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN); // Start UART for sensor
    Serial.println("Measuring distance...");
    lcd.init();
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(2);
}

void loop() {
    if (MySerial.available() >= 4) { // Check if at least 4 bytes are available
        uint8_t frame_header = MySerial.read();
        uint8_t data_H = MySerial.read();
        uint8_t data_L = MySerial.read();
        uint8_t received_checksum = MySerial.read();

        if (frame_header == 0xFF) { // Check the frame header
            uint8_t calculated_checksum = (frame_header + data_H + data_L) & 0xFF;

            if (calculated_checksum == received_checksum) {
                uint16_t distance_mm = (uint16_t)(data_H << 8) | data_L;
                float distance_m = distance_mm / 1000.0; // Convert mm to m
                // Eğer mesafe değiştiyse, ekranı güncelle
                if (distance_m != lastDistance) {
                    lcd.fillScreen(TFT_BLACK);
                    lcd.drawString("Measured Distance:", 10, 10);
                    lcd.drawString(String(distance_m) + " meters", 10, 30);
                    lastDistance = distance_m; // Son mesafeyi güncelle
                }
            } else {
                lcd.drawString("Checksum error", 10, 10);
            }
        } else {
            lcd.drawString("Invalid frame header", 10, 10);
        }
    }
    delay(100);
}

int main() {
    init(); // Initialize Arduino framework
    setup();
    while (true) {
        loop();
    }
    return 0;
}
