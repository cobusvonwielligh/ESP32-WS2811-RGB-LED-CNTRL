#include <Arduino.h>
#include "display.h"
#include "secrets.h"

/* ============================================================
      Display Utilities: SSD1306
---------------------------------------------------------------

      Provides helper functions to initialize the display
      and render different UI screens and graphics

============================================================== */

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static bool displayReady = false;

bool initDisplay() {
  if (displayReady) {
    return true;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[DISPLAY] SSD1306 allocation failed"));
    return false;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Display online"));
  display.display();

  displayReady = true;
  return true;
}

bool displayIsReady() {
  return displayReady;
}

void displayMessage(const String& message) {
  if (!displayReady) {
    Serial.println(F("[DISPLAY] Attempted to draw before init"));
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(message);
  display.display();
}
