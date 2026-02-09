//
// Created by Josemar Carvalho on 09/02/26.
//

#ifndef VEHICLE_DEVICE_STEPPER28BYJ48_H
#define VEHICLE_DEVICE_STEPPER28BYJ48_H

#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

class Stepper28BYJ48 {
public:
    struct Pins {
        uint8_t in1 = 14;
        uint8_t in2 = 27;
        uint8_t in3 = 26;
        uint8_t in4 = 25;
    };

    struct SpeedPotConfig {
        uint8_t adcPin = 35; // ADC1 recomendado
        uint8_t samples = 10; // média simples
        uint16_t updateMs = 100; // atualiza velocidade a cada X ms
        bool invert = false; // inverte o sentido do pot (se quiser)
    };

    struct MotionConfig {
        float minSpeed = 50.0f; // steps/s
        float maxSpeed = 900.0f; // steps/s
    };

    Stepper28BYJ48(const Pins &pins, const SpeedPotConfig &pot, const MotionConfig &motion);

    void begin();

    void update(); // chama no loop (não-bloqueante)

    float speed() const noexcept { return _speed; } // steps/s
    int speedRaw() const noexcept { return _speedRaw; } // 0..4095

private:
    Pins _pins{};
    SpeedPotConfig _pot{};
    MotionConfig _motion{};

    AccelStepper _stepper;
    uint32_t _lastUpdateMs = 0;

    int _speedRaw = 0;
    float _speed = 0.0f;

    int readAdcAveraged(uint8_t pin, uint8_t samples) const;

    float mapSpeed(int adcRaw) const;
};

#endif //VEHICLE_DEVICE_STEPPER28BYJ48_H
