#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp> 

// Create an instance of the LGFX object
static LGFX lcd;

void setup() {
  // Initialize the display
  lcd.init();
  
  // Fill the screen with black color
  lcd.fillScreen(TFT_BLACK);
  
  // Set the text color to white and the text size to 2
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  
  // Print "Hello, World!" to the center of the screen
  lcd.drawString("Hi, World!", lcd.width()/2 - 60, lcd.height()/2 - 10);
}

void loop() {
  // Nothing to do in the loop for this example
}

extern "C" void app_main() {
    setup();
    while(1) {
        loop();
        // It's good to add a small delay here, especially if your loop is empty.
        // This ensures that you don't unnecessarily hog the CPU.
        vTaskDelay(pdMS_TO_TICKS(10)); // delay for 10ms
    }
}
