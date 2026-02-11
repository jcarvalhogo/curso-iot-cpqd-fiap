//
// Created by Josemar Carvalho on 11/02/26.
//

#include "Dht22Sensor.h"

Dht22Sensor::Dht22Sensor(uint8_t pin, uint16_t minIntervalMs)
    : _pin(pin),
      _minIntervalMs(minIntervalMs),
      _dht(pin, DHT22) {
}

bool Dht22Sensor::begin() {
    _dht.begin();
    _lastAttemptMs = 0;
    _last = {false, NAN, NAN, 0};
    _lastErr = "";
    return true;
}

bool Dht22Sensor::read() {
    const uint32_t now = millis();
    if (_lastAttemptMs != 0 && (now - _lastAttemptMs) < _minIntervalMs) {
        return false;
    }
    _lastAttemptMs = now;

    const float h = _dht.readHumidity();
    const float t = _dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        _last = {false, NAN, NAN, now};
        _lastErr = "TIMEOUT/NaN";
        return true;
    }

    _last = {true, t, h, now};
    _lastErr = "";
    return true;
}

Dht22Sensor::Reading Dht22Sensor::last() const {
    return _last;
}

const char *Dht22Sensor::lastError() const {
    return _lastErr;
}
