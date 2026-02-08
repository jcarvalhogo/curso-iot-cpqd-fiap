//
// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_LEDSTATUS_H
#define GATEWAY_ARDUINO_LEDSTATUS_H

#pragma once
#include <Arduino.h>

class LedStatus {
public:
    enum class Mode {
        OFF, // LED apagado
        BLINK_FAST, // pisca rápido (ex: conectando)
        BLINK_SLOW, // pisca lento (ex: conectado)
        ON // LED ligado fixo
    };

    explicit LedStatus(uint8_t pin);

    void begin();

    void setMode(Mode mode);

    // Chamar sempre no loop() para atualizar o pisca sem bloquear
    void update();

private:
    uint8_t _pin;
    Mode _mode = Mode::OFF;
    uint32_t _lastToggle = 0;
    bool _level = false;

    uint32_t intervalMs() const;

    void write(bool on);
};

#endif //GATEWAY_ARDUINO_LEDSTATUS_H
