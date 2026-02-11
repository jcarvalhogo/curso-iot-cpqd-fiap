//
// Created by Josemar Carvalho on 11/02/26.
//

#include "LedRgbStatus.h"

LedRgbStatus::LedRgbStatus(uint8_t pin,
                           uint8_t brightness,
                           uint16_t pixelsCount,
                           neoPixelType type)
    : _pin(pin),
      _brightness(brightness),
      _pixelsCount(pixelsCount),
      _strip(pixelsCount, pin, type) {
}

void LedRgbStatus::begin() {
    _strip.begin();
    _strip.setBrightness(_brightness);
    _strip.clear();
    _strip.show();
    _lastMs = millis();
}

void LedRgbStatus::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    _strip.setBrightness(_brightness);
    _strip.show();
}

void LedRgbStatus::setOff() {
    _mode = Mode::Off;
    _color = Color::Off();
    showColor(0, 0, 0);
}

void LedRgbStatus::setSolid(Color c) {
    _mode = Mode::Solid;
    _color = c;
    showColor(c.r, c.g, c.b);
}

void LedRgbStatus::setBlink(Color c, uint16_t onMs, uint16_t offMs) {
    _mode = Mode::Blink;
    _color = c;
    _onMs = onMs;
    _offMs = offMs;
    _blinkOn = true;
    _lastMs = millis();
    showColor(c.r, c.g, c.b);
}

void LedRgbStatus::setPulse(Color c, uint16_t periodMs) {
    _mode = Mode::Pulse;
    _color = c;
    _periodMs = (periodMs == 0) ? 1 : periodMs;
    _lastMs = millis();
}

void LedRgbStatus::statusBoot() { setBlink(Color::Blue(), 150, 150); }
void LedRgbStatus::statusWifiConnecting() { setPulse(Color::Yellow(), 1200); }
void LedRgbStatus::statusOnline() { setSolid(Color::Green()); }
void LedRgbStatus::statusWarn() { setBlink(Color::Cyan(), 250, 750); }
void LedRgbStatus::statusError() { setBlink(Color::Red(), 120, 120); }

void LedRgbStatus::update() {
    const uint32_t now = millis();

    switch (_mode) {
        case Mode::Off:
        case Mode::Solid:
            return;

        case Mode::Blink: {
            const uint16_t wait = _blinkOn ? _onMs : _offMs;
            if (now - _lastMs >= wait) {
                _lastMs = now;
                _blinkOn = !_blinkOn;
                if (_blinkOn) showColor(_color.r, _color.g, _color.b);
                else showColor(0, 0, 0);
            }
            return;
        }

        case Mode::Pulse: {
            const uint32_t t = (now - _lastMs) % _periodMs;
            const float phase = (float) t / (float) _periodMs;
            const float tri = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
            const uint8_t k = (uint8_t)(tri * 255.0f);

            const uint8_t r = (uint8_t)((uint16_t) _color.r * k / 255);
            const uint8_t g = (uint8_t)((uint16_t) _color.g * k / 255);
            const uint8_t b = (uint8_t)((uint16_t) _color.b * k / 255);

            showColor(r, g, b);
            return;
        }
    }
}

void LedRgbStatus::showColor(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < _pixelsCount; i++) {
        _strip.setPixelColor(i, _strip.Color(r, g, b));
    }
    _strip.show();
}
