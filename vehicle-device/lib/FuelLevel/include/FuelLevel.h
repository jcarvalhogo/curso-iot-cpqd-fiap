//
// Created by Josemar Carvalho on 09/02/26.
//

#ifndef VEHICLE_DEVICE_FUELLEVEL_H
#define VEHICLE_DEVICE_FUELLEVEL_H

#pragma once
#include <Arduino.h>

class FuelLevel {
public:
    struct Config {
        uint8_t adcPin = 34; // ADC1 recomendado: 32-39 (ex: 34)
        int adcMin = 200; // calibração: leitura mínima real (pot no 0%)
        int adcMax = 3800; // calibração: leitura máxima real (pot no 100%)
        bool invert = false; // se o sentido ficar invertido
        uint8_t samples = 10; // média simples
    };

    explicit FuelLevel(const Config &cfg);

    void begin();

    int readPercent(); // 0..100
    int readRaw(); // 0..4095 (depende do ADC)

private:
    Config _cfg{};

    int readRawAveraged() const;
};


#endif //VEHICLE_DEVICE_FUELLEVEL_H
