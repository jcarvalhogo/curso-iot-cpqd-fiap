//
// Created by Josemar Carvalho on 10/02/26.
//

#ifndef VEHICLE_DEVICE_ACCELERATIONSIMULATOR_H
#define VEHICLE_DEVICE_ACCELERATIONSIMULATOR_H

#pragma once
#include <Arduino.h>

class AccelerationSimulator {
public:
    struct Config {
        uint8_t adcPin = 34; // ESP32 ADC1 default pin example
        uint16_t adcMax = 4095; // 12-bit
        float accelMin = 0.0f; // 0%
        float accelMax = 100.0f; // 100%
        uint8_t emaAlphaPct = 15; // 0..100 (suavização)
        uint16_t deadband = 10; // zona morta no ADC
    };

    explicit AccelerationSimulator(const Config &cfg);

    void begin();

    void update();

    // aceleração em % (0..100)
    float accelerationPct() const;

    // leitura bruta (0..adcMax)
    uint16_t raw() const;

private:
    uint16_t readRaw();

    float mapToPct(uint16_t v) const;

private:
    Config _cfg{};
    uint16_t _raw = 0;
    float _accelPct = 0.0f;
    bool _hasInit = false;
};


#endif //VEHICLE_DEVICE_ACCELERATIONSIMULATOR_H
