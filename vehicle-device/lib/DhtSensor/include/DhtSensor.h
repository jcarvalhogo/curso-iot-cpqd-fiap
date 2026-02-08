//
// Created by Josemar Carvalho on 08/02/26.
//

#ifndef VEHICLE_DEVICE_DHTSENSOR_H
#define VEHICLE_DEVICE_DHTSENSOR_H

#pragma once

#include <Arduino.h>
#include <DHTesp.h>

class DhtSensor {
public:
    enum class Type : uint8_t {
        DHT11,
        DHT22
    };

    struct Config {
        uint8_t pin = 4;              // ajuste conforme seu hardware
        Type type = Type::DHT22;       // padrão: DHT22
        uint32_t minIntervalMs = 2000; // DHTxx normalmente precisa >= 2s
    };

    struct Reading {
        bool valid = false;
        float temperature = NAN;
        float humidity = NAN;
        uint32_t lastReadMs = 0;
    };

    explicit DhtSensor(const Config& cfg);

    void begin();
    void update();       // chama no loop; lê respeitando minIntervalMs
    bool readNow();      // tenta ler agora (respeita minIntervalMs)

    bool hasData() const noexcept { return _data.valid; }
    const Reading& data() const noexcept { return _data; }

    void setDebugStream(Stream* s);

private:
    Config _cfg{};
    Stream* _dbg = nullptr;

    DHTesp _dht;
    Reading _data{};
    uint32_t _lastAttemptMs = 0;

    void dbgln(const String& s);
    bool doRead(uint32_t now);
};

#endif // VEHICLE_DEVICE_DHTSENSOR_H