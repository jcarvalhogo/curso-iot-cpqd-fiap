# LedStatus

A tiny **non-blocking status LED** helper for Arduino/ESP32 projects.

This library is meant to make a single LED communicate application state (booting, connecting, connected, error, etc.) **without using `delay()`**. It works by toggling the LED based on `millis()` timing.

> ⚠️ Note about your request: **LedStatus does not implement authentication**.  
> Authentication/encryption belongs to the `SecureHttp` library. LedStatus only drives a GPIO for visual status.

---

## What it does

- Drives one GPIO as an LED output
- Exposes simple modes:
  - `OFF`
  - `ON`
  - `BLINK_FAST` (default interval: 200 ms)
  - `BLINK_SLOW` (default interval: 1000 ms)
- Implements blinking with a small state machine using `millis()`
- Designed to be called from your main `loop()` (non-blocking)

---

## Guarantees / limitations

### Guarantees
- **Non-blocking** blinking (no `delay()`)
- Deterministic toggling interval, based on `millis()`
- Mode changes take effect immediately (`OFF`/`ON`) or on the next updates (blink)

### Limitations
- Assumes an **active-high LED** (`HIGH` turns LED on).  
  If your hardware uses active-low LED, you can:
  1) invert at wiring level, or  
  2) extend the library to support `activeLow` (see “Extending” below).

---

## Install / include

This library is PlatformIO-friendly (`library.json` present). In code:

```cpp
#include "LedStatus.h"
```

---

## Quick start

```cpp
#include <Arduino.h>
#include "LedStatus.h"

#define LED_PIN 2

LedStatus led(LED_PIN);

void setup() {
  Serial.begin(115200);
  led.begin();

  // Boot / connecting
  led.setMode(LedStatus::Mode::BLINK_FAST);
}

void loop() {
  // Call frequently
  led.update();

  // Example: when connected
  // led.setMode(LedStatus::Mode::BLINK_SLOW);

  // Example: error
  // led.setMode(LedStatus::Mode::ON);
}
```

---

## How it works (technical)

Internally, `LedStatus` stores:

- current `Mode`
- last toggle timestamp (`_lastToggle`)
- current output level (`_level`)

In `update()`:
1. If mode is `OFF` or `ON`, it returns immediately.
2. Otherwise it checks if `millis() - _lastToggle >= intervalMs()`.
3. If yes, it toggles `_level` and writes it to the GPIO.

This pattern ensures your main loop continues running (Wi-Fi, HTTP server, sensors, etc.) while the LED blinks.

---

## Extending (optional)

### Active-low LED support
If you want native support, add a boolean flag (e.g., `activeLow`) and invert `write()`:

```cpp
digitalWrite(_pin, activeLow ? (on ? LOW : HIGH) : (on ? HIGH : LOW));
```

### Custom blink intervals
Today, intervals are fixed inside `intervalMs()`:
- fast = 200 ms
- slow = 1000 ms

You can extend the class to accept custom intervals or add new modes (e.g., `BLINK_ERROR`).

---

## Doxygen

A Doxygen configuration is provided (`Doxyfile`). To generate HTML docs:

```bash
doxygen Doxyfile
```

Output (default): `docs/doxygen/html/index.html`

---

## License

Choose a license appropriate for your project (e.g., MIT/Apache-2.0).  
If you want, I can add a `LICENSE` file matching your repository’s standard.
