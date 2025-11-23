#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include "display.h"

// ----------------------------
// LED strip config
// ----------------------------
#define DATA_PIN 15  // GPIO14 (D5 on LOLIN32)
#define LED_TYPE WS2811
#define COLOR_ORDER BRG
#define NUM_LEDS 26         // <-- set to number of 3-LED groups on your strip
#define MAX_BRIGHTNESS 60  // 0–255, keep below 255 for safety

CRGB leds[NUM_LEDS];

// Capability map based on the user's physical strip (1-indexed in the notes)
// 1: no output, 2-10: red only, 11: full RGB, 12: red only, 13: red+green,
// 14: red only, 15: red+green, 16-22: red only, 23: full RGB, 24-26: red only.
enum Capability {
  CAP_NONE,
  CAP_RED,
  CAP_RED_GREEN,
  CAP_FULL
};

const Capability CAP_MAP[NUM_LEDS] = {
  CAP_NONE,       // 1
  CAP_RED,        // 2
  CAP_RED,        // 3
  CAP_RED,        // 4
  CAP_RED,        // 5
  CAP_RED,        // 6
  CAP_RED,        // 7
  CAP_RED,        // 8
  CAP_RED,        // 9
  CAP_RED,        // 10
  CAP_FULL,       // 11
  CAP_RED,        // 12
  CAP_RED_GREEN,  // 13
  CAP_RED,        // 14
  CAP_RED_GREEN,  // 15
  CAP_RED,        // 16
  CAP_RED,        // 17
  CAP_RED,        // 18
  CAP_RED,        // 19
  CAP_RED,        // 20
  CAP_RED,        // 21
  CAP_RED,        // 22
  CAP_FULL,       // 23
  CAP_RED,        // 24
  CAP_RED,        // 25
  CAP_RED         // 26
};

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
  MODE_STATIC_GREEN,
  MODE_STATIC_BLUE,
  MODE_STATIC_WHITE,
  MODE_RAINBOW,
  MODE_CHASE,
  MODE_HOME_STATUS,
  MODE_HEALTH_BAR,
  MODE_VU_METER,
  MODE_DUAL_BEACON,
  MODE_AMBER_WAVE,
  MODE_RED_SCANNER,
  MODE_SECTION_SPARKLE,
  MODE_CAPABILITY_DEMO
};

SystemState systemState = STATE_OFF;
Mode currentMode = MODE_RAINBOW;

struct ModeBinding {
  char key;
  Mode mode;
  const char* label;
  void (*handler)();
};

const ModeBinding MODES[] = {
  {'1', MODE_STATIC_RED, "STATIC RED", modeStaticRed},
  {'2', MODE_STATIC_GREEN, "STATIC GREEN", modeStaticGreen},
  {'3', MODE_STATIC_BLUE, "STATIC BLUE", modeStaticBlue},
  {'4', MODE_STATIC_WHITE, "STATIC WHITE", modeStaticWhite},
  {'5', MODE_RAINBOW, "RAINBOW", modeRainbow},
  {'6', MODE_CHASE, "CHASE", modeChase},
  {'7', MODE_HOME_STATUS, "HOME STATUS", modeHomeStatus},
  {'8', MODE_HEALTH_BAR, "HEALTH BAR", modeHealthBar},
  {'9', MODE_VU_METER, "VU METER", modeVuMeter},
  {'a', MODE_DUAL_BEACON, "DUAL BEACON", modeDualBeacon},
  {'b', MODE_AMBER_WAVE, "AMBER WAVE", modeAmberWave},
  {'c', MODE_RED_SCANNER, "RED SCAN", modeRedScanner},
  {'d', MODE_SECTION_SPARKLE, "SECTION SPARKLE", modeSectionSparkle},
  {'e', MODE_CAPABILITY_DEMO, "CAPABILITY DEMO", modeCapabilityDemo}
};

const size_t MODE_COUNT = sizeof(MODES) / sizeof(MODES[0]);

const ModeBinding* findModeByKey(char key) {
  for (size_t i = 0; i < MODE_COUNT; i++) {
    if (MODES[i].key == key) {
      return &MODES[i];
    }
  }
  return nullptr;
}

const ModeBinding* findModeById(Mode mode) {
  for (size_t i = 0; i < MODE_COUNT; i++) {
    if (MODES[i].mode == mode) {
      return &MODES[i];
    }
  }
  return nullptr;
}

const char* modeName(Mode mode) {
  const ModeBinding* binding = findModeById(mode);
  if (binding) {
    return binding->label;
  }
  return "UNKNOWN";
}

// ----------------------------
// Forward declarations
// ----------------------------
void updateOledStatus();
void setAll(const CRGB& c);
void setPixelWithCapability(uint8_t index, const CRGB& c);
CRGB applyCapabilities(uint8_t index, const CRGB& c);
void runCurrentMode();
void modeStaticRed();
void modeStaticGreen();
void modeStaticBlue();
void modeStaticWhite();
void modeRainbow();
void modeChase();
void modeHomeStatus();
void modeHealthBar();
void modeVuMeter();
void modeDualBeacon();
void modeAmberWave();
void modeRedScanner();
void modeSectionSparkle();
void modeCapabilityDemo();

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
  for (size_t i = 0; i < MODE_COUNT; i++) {
    Serial.print(' ');
    Serial.print(MODES[i].key);
    Serial.print(" = ");
    Serial.println(MODES[i].label);
  }
}

// ======================================================
// MAIN LOOP
// ======================================================
void loop() {
  // --- simple Serial control for modes ---
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '0') {
      systemState = STATE_OFF;
      setAll(CRGB::Black);
      FastLED.show();
    } else {
      const ModeBinding* binding = findModeByKey(c);
      if (binding) {
        systemState = STATE_ON;
        currentMode = binding->mode;
      } else {
        systemState = STATE_ERROR;
      }
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
    case STATE_OFF: stateStr = "OFF"; break;
    case STATE_ON: stateStr = "ON"; break;
    case STATE_ERROR: stateStr = "ERROR"; break;
  }

  String msg = "LED CTRL\n";
  msg += "State: " + stateStr + "\n";
  msg += "Mode : " + String(modeName(currentMode)) + "\n";
  msg += "N_PIX: ";
  msg += NUM_LEDS;

  displayMessage(msg);
}

// ======================================================
// LED utility functions
// ======================================================
CRGB applyCapabilities(uint8_t index, const CRGB& c) {
  Capability cap = CAP_MAP[index];
  switch (cap) {
    case CAP_NONE:
      return CRGB::Black;
    case CAP_RED:
      return CRGB(c.r, 0, 0);
    case CAP_RED_GREEN:
      return CRGB(c.r, c.g, 0);
    case CAP_FULL:
    default:
      return c;
  }
}

void setPixelWithCapability(uint8_t index, const CRGB& c) {
  if (index >= NUM_LEDS) return;
  leds[index] = applyCapabilities(index, c);
}

void setAll(const CRGB& c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelWithCapability(i, c);
  }
}

// Dispatch current mode
void runCurrentMode() {
  const ModeBinding* binding = findModeById(currentMode);
  if (binding && binding->handler) {
    binding->handler();
  }
}

// ----------------- MODES -----------------
// 1) Simple static red
void modeStaticRed() {
  setAll(CRGB::Red);
  FastLED.show();
  delay(20);
}

// 2) Simple static green
void modeStaticGreen() {
  setAll(CRGB::Green);
  FastLED.show();
  delay(20);
}

// 3) Simple static blue
void modeStaticBlue() {
  setAll(CRGB::Blue);
  FastLED.show();
  delay(20);
}

// 4) Simple static White
void modeStaticWhite() {
  setAll(CRGB::White);
  FastLED.show();
  delay(20);
}

// 5) Classic rainbow
void modeRainbow() {
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelWithCapability(i, CHSV(hue + i * 8, 255, 255));
  }
  FastLED.show();
  hue++;
  delay(20);
}

// 6) Moving "chase" dot
void modeChase() {
  static int pos = 0;
  setAll(CRGB::Black);
  setPixelWithCapability(pos, CRGB::Blue);
  FastLED.show();

  pos++;
  if (pos >= NUM_LEDS) pos = 0;
  delay(50);
}

// 7) Home status pulse: red ambient with state color on the two full RGB sections
void modeHomeStatus() {
  static uint8_t status = 0;  // 0=OK,1=WARNING,2=ALERT
  static uint32_t lastChange = 0;
  uint32_t now = millis();
  if (now - lastChange > 5000) {
    status = (status + 1) % 3;
    lastChange = now;
  }

  uint8_t base = beatsin8(6, 5, 40);
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelWithCapability(i, CRGB(base, 0, 0));
  }

  CRGB statusColor;
  switch (status) {
    case 0: statusColor = CRGB::Green; break;       // OK
    case 1: statusColor = CRGB(255, 170, 0); break; // Warning amber
    default: statusColor = CRGB::Red; break;        // Alert
  }

  setPixelWithCapability(10, statusColor);  // Section 11
  setPixelWithCapability(22, statusColor);  // Section 23
  FastLED.show();
  delay(20);
}

// 8) Health bar: fills from left to right using available colors
void modeHealthBar() {
  uint8_t level = beatsin8(4, 0, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++) {
    bool filled = i < level;
    Capability cap = CAP_MAP[i];
    CRGB color = CRGB(10, 0, 0);  // dim red baseline

    if (filled) {
      if (cap == CAP_FULL) {
        // Healthy spots glow teal to stand out
        uint8_t hue = map(i, 0, NUM_LEDS - 1, 96, 140);
        color = CHSV(hue, 200, 220);
      } else if (cap == CAP_RED_GREEN) {
        color = CRGB(255, 160, 0);  // Amber for mid capability
      } else if (cap == CAP_RED) {
        color = CRGB::Red;
      }
    }

    setPixelWithCapability(i, color);
  }

  FastLED.show();
  delay(30);
}

// 9) Simulated VU meter with decaying peak indicator
void modeVuMeter() {
  static uint8_t peak = 0;
  uint8_t level = beatsin8(14, 0, NUM_LEDS);  // Fast-moving pseudo audio
  if (level > peak) {
    peak = level;
  } else if (peak > 0) {
    peak--;
  }

  setAll(CRGB::Black);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i <= level) {
      Capability cap = CAP_MAP[i];
      CRGB color = CRGB::Red;
      if (cap == CAP_FULL) {
        // Cool tones for the rare full sections
        color = CRGB(0, 180, 220);
      } else if (cap == CAP_RED_GREEN) {
        color = CRGB(255, 200, 40);
      }
      setPixelWithCapability(i, color);
    }
  }

  if (peak < NUM_LEDS) {
    CRGB peakColor = (CAP_MAP[peak] == CAP_FULL) ? CRGB::White : CRGB(255, 120, 0);
    setPixelWithCapability(peak, peakColor);
  }

  FastLED.show();
  delay(30);
}

// 10) Dual beacon: red ribbon with alternating cyan/magenta flashes on RGB sections
void modeDualBeacon() {
  uint8_t breathe = beatsin8(3, 10, 80);
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelWithCapability(i, CRGB(breathe, 0, 0));
  }

  uint8_t flash = beatsin8(7, 120, 255);
  bool alt = ((millis() / 900) % 2) == 0;
  setPixelWithCapability(10, alt ? CRGB(0, flash, flash) : CRGB(flash, 0, flash));
  setPixelWithCapability(22, alt ? CRGB(flash, 0, flash) : CRGB(0, flash, flash));

  // Soften the ends to feel like beacons
  setPixelWithCapability(1, CRGB(120, 0, 0));
  setPixelWithCapability(NUM_LEDS - 1, CRGB(120, 0, 0));

  FastLED.show();
  delay(20);
}

// 11) Amber wave: slow crawling amber that respects red/amber-only areas
void modeAmberWave() {
  static uint8_t offset = 0;
  offset++;

  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t intensity = sin8(offset + i * 10);
    CRGB color = CRGB(intensity, intensity / 2, 0);
    setPixelWithCapability(i, color);
  }

  // Let the RGB islands pop a bit more
  setPixelWithCapability(10, CHSV(32, 150, beatsin8(5, 80, 200)));
  setPixelWithCapability(22, CHSV(32, 150, beatsin8(5, 80, 200)));

  FastLED.show();
  delay(30);
}

// 12) Red scanner with tapering tail
void modeRedScanner() {
  static int pos = 0;
  static int dir = 1;

  setAll(CRGB::Black);
  for (int t = 0; t < 4; t++) {
    int idx = pos - t * dir;
    if (idx >= 0 && idx < NUM_LEDS) {
      uint8_t level = 200 - t * 40;
      setPixelWithCapability(idx, CRGB(level, 0, 0));
    }
  }

  pos += dir;
  if (pos >= NUM_LEDS - 1 || pos <= 0) {
    dir *= -1;
  }

  FastLED.show();
  delay(25);
}

// 13) Section sparkle: random cool sparkles on capable sections
void modeSectionSparkle() {
  fadeToBlackBy(leds, NUM_LEDS, 40);

  uint8_t chance = random8(0, 5);
  if (chance == 0) {
    uint8_t idx = random8(0, NUM_LEDS);
    Capability cap = CAP_MAP[idx];
    CRGB color = CRGB::Red;
    if (cap == CAP_FULL) {
      color = CRGB(0, 160, 255);
    } else if (cap == CAP_RED_GREEN) {
      color = CRGB(255, 200, 20);
    }
    setPixelWithCapability(idx, color);
  }

  FastLED.show();
  delay(20);
}

// 14) Capability demo: shows what each section can display
void modeCapabilityDemo() {
  uint8_t pulse = beatsin8(5, 30, 200);
  for (int i = 0; i < NUM_LEDS; i++) {
    Capability cap = CAP_MAP[i];
    CRGB color = CRGB::Black;
    switch (cap) {
      case CAP_NONE: color = CRGB::Black; break;
      case CAP_RED: color = CRGB(pulse, 0, 0); break;
      case CAP_RED_GREEN: color = CRGB(pulse, pulse / 2, 0); break;
      case CAP_FULL: color = CHSV(140, 200, pulse); break;
    }
    setPixelWithCapability(i, color);
  }

  FastLED.show();
  delay(30);
}
