//
// Created by Josemar Carvalho on 10/02/26.
//

#include "AccelerationSimulator.h"

AccelerationSimulator::AccelerationSimulator(const Config &cfg) : _cfg(cfg) {
}

void AccelerationSimulator::begin() {
    pinMode(_cfg.adcPin, INPUT);
    _raw = readRaw();
    _accelPct = mapToPct(_raw);
    _hasInit = true;
}

uint16_t AccelerationSimulator::readRaw() {
    // ESP32: analogRead já retorna 0..4095 (por padrão 12-bit)
    int v = analogRead(_cfg.adcPin);
    if (v < 0) v = 0;
    if (v > _cfg.adcMax) v = _cfg.adcMax;
    return (uint16_t) v;
}

float AccelerationSimulator::mapToPct(uint16_t v) const {
    // deadband simples perto do valor anterior
    // (aplicado no update, aqui é só map)
    const float t = (float) v / (float) _cfg.adcMax; // 0..1
    return _cfg.accelMin + t * (_cfg.accelMax - _cfg.accelMin);
}

void AccelerationSimulator::update() {
    const uint16_t newRaw = readRaw();

    if (_hasInit && (abs((int) newRaw - (int) _raw) <= _cfg.deadband)) {
        return;
    }

    _raw = newRaw;
    const float target = mapToPct(_raw);

    // EMA: y = a*x + (1-a)*y
    const float a = (float) _cfg.emaAlphaPct / 100.0f;
    _accelPct = _hasInit ? (a * target + (1.0f - a) * _accelPct) : target;

    _hasInit = true;
}

float AccelerationSimulator::accelerationPct() const { return _accelPct; }
uint16_t AccelerationSimulator::raw() const { return _raw; }
