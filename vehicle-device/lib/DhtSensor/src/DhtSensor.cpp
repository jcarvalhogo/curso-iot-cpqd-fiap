//
// Created by Josemar Carvalho on 08/02/26.
//

#include "DhtSensor.h"

DhtSensor::DhtSensor(const Config &cfg) : _cfg(cfg) {
}

void DhtSensor::setDebugStream(Stream *s) {
    _dbg = s;
}

void DhtSensor::dbgln(const String &s) {
    if (_dbg) _dbg->println(s);
}

void DhtSensor::begin() {
    const auto dhtType = (_cfg.type == Type::DHT11) ? DHTesp::DHT11 : DHTesp::DHT22;
    _dht.setup(_cfg.pin, dhtType);

    dbgln(String("[DHT] begin pin=") + _cfg.pin + " type=" + ((_cfg.type == Type::DHT11) ? "DHT11" : "DHT22"));
}

void DhtSensor::update() {
    const uint32_t now = millis();
    (void) doRead(now);
}

bool DhtSensor::readNow() {
    const uint32_t now = millis();
    return doRead(now);
}

bool DhtSensor::doRead(uint32_t now) {
    // evita leituras rápidas demais (DHT precisa de intervalo)
    if (_lastAttemptMs != 0 && (now - _lastAttemptMs) < _cfg.minIntervalMs) {
        return false;
    }
    _lastAttemptMs = now;

    TempAndHumidity th = _dht.getTempAndHumidity();

    // DHTesp retorna NAN quando falha
    if (isnan(th.temperature) || isnan(th.humidity)) {
        _data.valid = false;
        _data.temperature = NAN;
        _data.humidity = NAN;
        _data.lastReadMs = now;

        dbgln("[DHT] read failed (NaN). Check wiring/pin/pull-up/power.");
        return false;
    }

    _data.valid = true;
    _data.temperature = th.temperature;
    _data.humidity = th.humidity;
    _data.lastReadMs = now;

    return true;
}
