#pragma once
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET false
#define OLED_ADDR 0x3C

/* =========================================
 *        SSD1306 OLED Display Driver
 * ========================================= */


extern Adafruit_SSD1306 display;

void initDisplay(Adafruit_SSD1306& d);
void displayMessage(String message);