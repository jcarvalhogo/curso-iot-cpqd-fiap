//
// Created by Josemar Carvalho on 07/02/26.
//

#include "LedStatus.h"

LedStatus::LedStatus(uint8_t pin) : _pin(pin) {
}

void LedStatus::begin() {
    pinMode(_pin, OUTPUT);
    write(false);
}

void LedStatus::setMode(Mode mode) {
    _mode = mode;
    _lastToggle = millis();

    // Ajuste imediato dependendo do modo
    if (_mode == Mode::OFF) write(false);
    if (_mode == Mode::ON) write(true);
}

void LedStatus::update() {
    // Modos fixos não precisam de atualização
    if (_mode == Mode::OFF || _mode == Mode::ON) return;

    uint32_t now = millis();
    if (now - _lastToggle >= intervalMs()) {
        _lastToggle = now;
        _level = !_level;
        write(_level);
    }
}

uint32_t LedStatus::intervalMs() const {
    switch (_mode) {
        case Mode::BLINK_FAST: return 200;
        case Mode::BLINK_SLOW: return 1000;
        default: return 1000;
    }
}

void LedStatus::write(bool on) {
    _level = on;
    digitalWrite(_pin, on ? HIGH : LOW);
}
