#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include "display.h"

// ----------------------------
// LED strip config
// ----------------------------
#define DATA_PIN       15          // GPIO14 (D5 on LOLIN32)
#define LED_TYPE       WS2811
#define COLOR_ORDER    GRB
#define NUM_LEDS       26          // <-- set to number of 3-LED groups on your strip
#define MAX_BRIGHTNESS 180         // 0–255, keep below 255 for safety

CRGB leds[NUM_LEDS];

// ----------------------------
// System state / modes
// ----------------------------
enum SystemState {
  STATE_OFF,
  STATE_ON,
  STATE_ERROR
};

enum Mode {
  MODE_STATIC_RED,
  MODE_RAINBOW,
  MODE_CHASE
};

SystemState systemState = STATE_OFF;
Mode currentMode       = MODE_STATIC_RED;

// ----------------------------
// Forward declarations
// ----------------------------
void updateOledStatus();
void setAll(const CRGB& c);
void runCurrentMode();
void modeStaticRed();
void modeRainbow();
void modeChase();

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- OLED / I2C ---
  Wire.begin();
  bool displayOk = initDisplay();
  if (!displayOk) {
    Serial.println(F("[BOOT] OLED init failed - check wiring"));
    systemState = STATE_ERROR;
  } else {
    displayMessage("Booting...\nLED Controller");
  }

  // --- FastLED strip init ---
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(MAX_BRIGHTNESS);

  // Start with LEDs off
  setAll(CRGB::Black);
  FastLED.show();

  if (systemState != STATE_ERROR) {
    systemState = STATE_OFF;
  }
  updateOledStatus();

  Serial.println("Ready. Commands:");
  Serial.println(" 0 = OFF");
  Serial.println(" 1 = STATIC RED");
  Serial.println(" 2 = RAINBOW");
  Serial.println(" 3 = CHASE");
}

// ======================================================
// MAIN LOOP
// ======================================================
void loop() {
  // --- simple Serial control for modes ---
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '0':
        systemState = STATE_OFF;
        setAll(CRGB::Black);
        FastLED.show();
        break;
      case '1':
        systemState = STATE_ON;
        currentMode = MODE_STATIC_RED;
        break;
      case '2':
        systemState = STATE_ON;
        currentMode = MODE_RAINBOW;
        break;
      case '3':
        systemState = STATE_ON;
        currentMode = MODE_CHASE;
        break;
      default:
        systemState = STATE_ERROR;
        break;
    }
    updateOledStatus();
  }

  // --- run animation if ON ---
  if (systemState == STATE_ON) {
    runCurrentMode();
  }

  // If ERROR, just show a quick red flash pattern
  if (systemState == STATE_ERROR) {
    static uint32_t lastToggle = 0;
    uint32_t now = millis();
    if (now - lastToggle > 300) {
      static bool on = false;
      on = !on;
      setAll(on ? CRGB::Red : CRGB::Black);
      FastLED.show();
      lastToggle = now;
    }
  }
}

// ======================================================
// OLED helpers
// ======================================================
void updateOledStatus() {
  if (!displayIsReady()) {
    Serial.println(F("[DISPLAY] Not ready"));
    return;
  }

  String stateStr;
  switch (systemState) {
    case STATE_OFF:   stateStr = "OFF";   break;
    case STATE_ON:    stateStr = "ON";    break;
    case STATE_ERROR: stateStr = "ERROR"; break;
  }

  String modeStr;
  switch (currentMode) {
    case MODE_STATIC_RED: modeStr = "STATIC RED"; break;
    case MODE_RAINBOW:    modeStr = "RAINBOW";    break;
    case MODE_CHASE:      modeStr = "CHASE";      break;
  }

  String msg = "LED CTRL\n";
  msg += "State: " + stateStr + "\n";
  msg += "Mode : " + modeStr + "\n";
  msg += "N_PIX: ";
  msg += NUM_LEDS;

  displayMessage(msg);
}

// ======================================================
// LED utility functions
// ======================================================
void setAll(const CRGB& c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = c;
  }
}

// Dispatch current mode
void runCurrentMode() {
  switch (currentMode) {
    case MODE_STATIC_RED: modeStaticRed(); break;
    case MODE_RAINBOW:    modeRainbow();   break;
    case MODE_CHASE:      modeChase();     break;
  }
}

// ----------------- MODES -----------------
// 1) Simple static red
void modeStaticRed() {
  setAll(CRGB::Red);
  FastLED.show();
  delay(20);
}

// 2) Classic rainbow
void modeRainbow() {
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + i * 8, 255, 255);
  }
  FastLED.show();
  hue++;
  delay(20);
}

// 3) Moving "chase" dot
void modeChase() {
  static int pos = 0;
  setAll(CRGB::Black);
  leds[pos] = CRGB::Blue;
  FastLED.show();

  pos++;
  if (pos >= NUM_LEDS) pos = 0;
  delay(50);
}
