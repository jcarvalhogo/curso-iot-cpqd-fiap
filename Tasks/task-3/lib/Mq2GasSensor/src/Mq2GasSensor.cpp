//
// Created by Josemar Carvalho on 11/02/26.
//

#include "Mq2GasSensor.h"
#include <math.h>

static constexpr float PPM_MIN = 0.1f;
static constexpr float PPM_MAX = 100000.0f;

static inline float clampPpm(float v) {
    if (!isfinite(v)) return NAN;
    if (v < PPM_MIN) return PPM_MIN;
    if (v > PPM_MAX) return PPM_MAX;
    return v;
}

Mq2GasSensor::Mq2GasSensor(uint8_t aoutPin) : _pin(aoutPin) {
}

bool Mq2GasSensor::begin() {
    pinMode(_pin, INPUT);

#if defined(ESP32)
    // ADC_11db ~ até perto de 3.3V (bom para AOUT com divisor / limit)
    analogSetPinAttenuation(_pin, ADC_11db);
#endif

    return true;
}

void Mq2GasSensor::setMaxVoltage(float maxVoltage) {
    if (maxVoltage < 0.1f) maxVoltage = 0.1f;
    _maxVoltage = maxVoltage;
}

Mq2GasSensor::Reading Mq2GasSensor::read(uint8_t samples, uint16_t sampleDelayMs) {
    if (samples == 0) samples = 1;

    uint32_t acc = 0;
    for (uint8_t i = 0; i < samples; i++) {
        acc += (uint16_t) analogRead(_pin);
        if (sampleDelayMs) delay(sampleDelayMs);
    }

    const uint16_t adc = (uint16_t) (acc / samples);
    const float voltage = ((float) adc * _vref) / (float) _adcMax;

    float norm = voltage / _maxVoltage;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    _last = {true, adc, voltage, norm, millis()};
    return _last;
}

// Rs = RL * (Vcc - Vout) / Vout
float Mq2GasSensor::rsKOhmFromVoltage(float vout) const {
    if (vout <= 0.0001f) return INFINITY;
    if (vout >= (_vcc - 0.0001f)) return 0.0f;
    return _rlKOhm * ((_vcc - vout) / vout);
}

float Mq2GasSensor::rsKOhmFromAdc(uint16_t adc) const {
    const float vout = ((float) adc * _vref) / (float) _adcMax;
    return rsKOhmFromVoltage(vout);
}

float Mq2GasSensor::rsRoRatioFromVoltage(float vout) const {
    if (!_hasR0 || isnan(_r0KOhm) || _r0KOhm <= 0.0f) return NAN;
    const float rs = rsKOhmFromVoltage(vout);
    return rs / _r0KOhm;
}

float Mq2GasSensor::rsRoRatioFromAdc(uint16_t adc) const {
    const float vout = ((float) adc * _vref) / (float) _adcMax;
    return rsRoRatioFromVoltage(vout);
}

bool Mq2GasSensor::calibrateCleanAir(uint8_t samples, uint16_t sampleDelayMs, float cleanAirFactor) {
    if (cleanAirFactor <= 0.1f) cleanAirFactor = 9.83f;

    const auto r = read(samples, sampleDelayMs);
    const float rs = rsKOhmFromVoltage(r.voltage);

    if (!isfinite(rs) || rs <= 0.0f) {
        _hasR0 = false;
        _r0KOhm = NAN;
        return false;
    }

    _r0KOhm = rs / cleanAirFactor;
    _hasR0 = isfinite(_r0KOhm) && _r0KOhm > 0.0f;
    return _hasR0;
}

float Mq2GasSensor::ppmFromCurve(float rsRo, const Curve &c) {
    if (!isfinite(rsRo) || rsRo <= 0.0f) return NAN;

    const float logRsRo = log10f(rsRo);
    const float logPpm = ((logRsRo - c.y) / c.slope) + c.x;

    float ppm = powf(10.0f, logPpm);
    return clampPpm(ppm);
}

// ✅ NOVO: gera PPM “simulado” a partir do normalized (0..1),
// usando escala log para variar de 0.1 até 100000.
float Mq2GasSensor::ppmSimFromNormalized(float normalized) const {
    if (!isfinite(normalized)) return NAN;

    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;

    const float logMin = log10f(PPM_MIN); // -1
    const float logMax = log10f(PPM_MAX); // 5
    const float logPpm = logMin + normalized * (logMax - logMin);

    const float ppm = powf(10.0f, logPpm);
    return clampPpm(ppm);
}

float Mq2GasSensor::ppm(Gas gas) const {
    if (!_last.ok) return NAN;
    const float rsRo = rsRoRatioFromVoltage(_last.voltage);
    return ppm(gas, rsRo);
}

float Mq2GasSensor::ppm(Gas gas, float rsRo) const {
    static const Curve LPG{2.3f, 0.21f, -0.47f};
    static const Curve CO{2.3f, 0.72f, -0.34f};
    static const Curve SMOKE{2.3f, 0.53f, -0.44f};
    static const Curve H2{2.3f, 0.93f, -1.44f};

    switch (gas) {
        case Gas::LPG: return ppmFromCurve(rsRo, LPG);
        case Gas::CO: return ppmFromCurve(rsRo, CO);
        case Gas::SMOKE: return ppmFromCurve(rsRo, SMOKE);
        case Gas::H2: return ppmFromCurve(rsRo, H2);
    }
    return NAN;
}
