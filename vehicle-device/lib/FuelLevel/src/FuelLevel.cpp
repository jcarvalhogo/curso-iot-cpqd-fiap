//
// Created by Josemar Carvalho on 09/02/26.
//
#include "FuelLevel.h"

FuelLevel::FuelLevel(const Config &cfg) : _cfg(cfg) {
}

void FuelLevel::begin() {
    pinMode(_cfg.adcPin, INPUT);
}

int FuelLevel::readRawAveraged() const {
    long sum = 0;
    const uint8_t n = (_cfg.samples == 0) ? 1 : _cfg.samples;

    for (uint8_t i = 0; i < n; i++) {
        sum += analogRead(_cfg.adcPin);
        delay(2);
    }
    return (int) (sum / n);
}

int FuelLevel::readRaw() {
    return analogRead(_cfg.adcPin);
}

int FuelLevel::readPercent() {
    int raw = readRawAveraged();

    int minV = _cfg.adcMin;
    int maxV = _cfg.adcMax;
    if (maxV <= minV) {
        // fallback seguro se a calibração estiver errada
        minV = 0;
        maxV = 4095;
    }

    raw = constrain(raw, minV, maxV);

    long pct = map(raw, minV, maxV, 0, 100);
    if (_cfg.invert) pct = 100 - pct;

    return (int) constrain((int) pct, 0, 100);
}
