# ESP32-WS2811-RGB-LED-CNTRL
WS2811 LED strip controller for ESP32-WROOM-32

## Hardware-aware section map
The controller now understands your strip's uneven color support so animations only request colors each section can show:

* 1: no output
* 2–10: red only
* 11: full RGB
* 12: red only
* 13: red + green
* 14: red only
* 15: red + green
* 16–22: red only
* 23: full RGB
* 24–26: red only

## Mode list (Serial shortcuts)
Send the listed character over Serial to change modes. `0` always turns everything off.

### Functional indicators
* `7` **HOME STATUS** – Red ambient glow with the two RGB islands showing cycling status colors (green = OK, amber = warning, red = alert). Designed to later map to Home Assistant states.
* `8` **HEALTH BAR** – Fills from left to right using available colors: red-only sections show urgency, amber sits on red+green sections, and the RGB islands glow teal for the healthiest portion.
* `9` **VU METER** – Simulated audio meter that grows quickly with a decaying peak marker. Uses cool colors on RGB-capable sections to differentiate them from the red-only bed.

### Graphical showcases
* `a` **DUAL BEACON** – Red ribbon with alternating cyan/magenta flashes on the RGB islands and brighter endcaps.
* `b` **AMBER WAVE** – Slow crawling amber wave optimized for red/amber-only sections, with extra pop on the RGB islands.
* `c` **RED SCAN** – Knight Rider style scanner that respects the strip's red emphasis.
* `d` **SECTION SPARKLE** – Random sparkles that turn blue on RGB sections, amber on red+green sections, and red elsewhere.
* `e` **CAPABILITY DEMO** – Pulsing visualization that highlights what each section can display (red, amber, or teal for full RGB).

### Legacy static/utility modes
* `1` Static red
* `2` Static green
* `3` Static blue
* `4` Static white
* `5` Rainbow
* `6` Chase dot
