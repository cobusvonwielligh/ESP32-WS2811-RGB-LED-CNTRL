#include "display.h"
#include "secrets.h"

/* ============================================================
      Display Utilities: SSD1306
--------------------------------------------------------------- 
   
      Provides helper functions to initialize the display 
      and render different UI screens and graphics

============================================================== */


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static Adafruit_SSD1306* displayRef = nullptr;

void initDisplay(Adafruit_SSD1306& d) {
  displayRef = &d;
}

void displayMessage(String message) {
  Adafruit_SSD1306& display = *displayRef;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(message);
  display.display();
}