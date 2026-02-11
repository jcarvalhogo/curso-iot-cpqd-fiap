//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_3_MQ2GASSENSOR_H
#define TASK_3_MQ2GASSENSOR_H

#pragma once

#include <Arduino.h>

class Mq2GasSensor {
public:
    enum class Gas : uint8_t {
        LPG,
        CO,
        SMOKE,
        H2
    };

    struct Reading {
        bool ok;
        uint16_t adc; // 0..4095 (ESP32 12-bit)
        float voltage; // V (estimada com base em vref/adcMax)
        float normalized; // 0..1 (clamp)
        uint32_t tsMs;
    };

    // pin: ADC1 recomendado (ex.: 34)
    explicit Mq2GasSensor(uint8_t aoutPin = 34);

    bool begin();

    float ppmSimFromNormalized(float normalized) const;

    // --- config elétrica / ADC ---
    void setVref(float vref) { _vref = vref; } // padrão 3.3
    void setAdcMax(uint16_t adcMax) { _adcMax = adcMax; } // padrão 4095
    void setVcc(float vcc) { _vcc = vcc; } // padrão 5.0 (módulo geralmente em 5V)
    void setLoadResistanceKOhm(float rlKOhm) { _rlKOhm = rlKOhm; }

    // padrão 5k (curva datasheet usa RL=5k) :contentReference[oaicite:1]{index=1}
    void setMaxVoltage(float maxVoltage); // para normalized()

    // --- leitura ---
    Reading read(uint8_t samples = 10, uint16_t sampleDelayMs = 5);

    // --- MQ2 físico (Rs/R0) ---
    float rsKOhmFromVoltage(float vout) const; // Rs em kOhm
    float rsKOhmFromAdc(uint16_t adc) const;

    float rsRoRatioFromVoltage(float vout) const;

    float rsRoRatioFromAdc(uint16_t adc) const;

    // --- calibração ---
    // Ar limpo: R0 = Rs / cleanAirFactor (≈ 9.83) :contentReference[oaicite:2]{index=2}
    bool calibrateCleanAir(uint8_t samples = 50, uint16_t sampleDelayMs = 50, float cleanAirFactor = 9.83f);

    bool hasR0() const { return _hasR0; }
    float r0KOhm() const { return _r0KOhm; }

    // --- PPM (estimado) ---
    float ppm(Gas gas) const; // usa a última leitura
    float ppm(Gas gas, float rsRo) const; // permite passar rs/ro

    float ppmLpg() const { return ppm(Gas::LPG); }
    float ppmCO() const { return ppm(Gas::CO); }
    float ppmSmoke() const { return ppm(Gas::SMOKE); }
    float ppmH2() const { return ppm(Gas::H2); }

    uint8_t pin() const { return _pin; }

private:
    // Curva no formato do exemplo clássico: { x, y, slope }
    // onde x=log10(ppm1), y=log10(rs/ro em ppm1), slope = (y2-y1)/(x2-x1)
    struct Curve {
        float x;
        float y;
        float slope;
    };

    static float ppmFromCurve(float rsRo, const Curve &c);

private:
    uint8_t _pin;

    // ADC
    float _vref = 3.3f;
    uint16_t _adcMax = 4095;
    float _maxVoltage = 3.3f;

    // Elétrico do módulo MQ2
    float _vcc = 5.0f;
    float _rlKOhm = 5.0f; // RL=5k aparece na condição da curva do datasheet :contentReference[oaicite:3]{index=3}

    // Calibração
    bool _hasR0 = false;
    float _r0KOhm = NAN;

    // Cache última leitura
    Reading _last{false, 0, 0.0f, 0.0f, 0};
};


#endif //TASK_3_MQ2GASSENSOR_H
