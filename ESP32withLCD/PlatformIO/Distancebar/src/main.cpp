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

const float MIN_DISTANCE = 0.25; // Minimum distance for the bar (in meters)
const float MAX_DISTANCE = 7.5;  // Maximum distance for the bar (in meters)
const int BAR_LENGTH = 300;      // Total length of the bar in pixels
const int BAR_HEIGHT = 20;       // Height of the bar in pixels
const int BAR_X = 10;            // X position of the bar
const int BAR_Y = 50;            // Y position of the bar

float lastDistance = -1.0;

void setup() {
    Serial.begin(115200); 
    MySerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    lcd.init();
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(2);
}

void drawBar(float distance) {
    // Map the distance to the bar length
    int barFill = map(distance * 1000, MIN_DISTANCE * 1000, MAX_DISTANCE * 1000, 0, BAR_LENGTH);

    // Draw the empty bar
    lcd.drawRect(BAR_X, BAR_Y, BAR_LENGTH, BAR_HEIGHT, TFT_WHITE);

    // Fill the bar according to the measured distance
    if (barFill > 0) {
        lcd.fillRect(BAR_X, BAR_Y, barFill, BAR_HEIGHT, TFT_GREEN);
    }
}

void loop() {
    if (MySerial.available() >= 4) { 
        uint8_t frame_header = MySerial.read();
        uint8_t data_H = MySerial.read();
        uint8_t data_L = MySerial.read();
        uint8_t received_checksum = MySerial.read();

        if (frame_header == 0xFF) {
            uint8_t calculated_checksum = (frame_header + data_H + data_L) & 0xFF;

            if (calculated_checksum == received_checksum) {
                uint16_t distance_mm = (uint16_t)(data_H << 8) | data_L;
                float distance_m = distance_mm / 1000.0; 

                if (distance_m != lastDistance) {
                    lcd.fillScreen(TFT_BLACK);
                    lcd.drawString("Measured Distance:", 10, 10);
                    lcd.drawString(String(distance_m) + " meters", 10, 30);

                    drawBar(distance_m); // Draw the bar

                    lastDistance = distance_m; 
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
    init();
    setup();
    while (true) {
        loop();
    }
    return 0;
}
