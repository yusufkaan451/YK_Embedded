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
  lcd.drawString("Hello, World!", lcd.width()/2 - 60, lcd.height()/2 - 10);
}

void loop() {
  // Nothing to do in the loop for this example
}
