//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_03_DHT22SENSOR_H
#define TASK_03_DHT22SENSOR_H

#pragma once

#include <Arduino.h>
#include <DHT.h>

class Dht22Sensor {
public:
    struct Reading {
        bool ok;
        float temperatureC;
        float humidity;
        uint32_t tsMs;
    };

    explicit Dht22Sensor(uint8_t pin, uint16_t minIntervalMs = 2000);

    bool begin();

    bool read();

    Reading last() const;

    const char *lastError() const;

private:
    uint8_t _pin;
    uint16_t _minIntervalMs;

    DHT _dht;

    uint32_t _lastAttemptMs = 0;
    Reading _last{false, NAN, NAN, 0};
    const char *_lastErr = "";
};

#endif //TASK_03_DHT22SENSOR_H
