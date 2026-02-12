//
// Created by Josemar Carvalho on 11/02/26.
//

#include "RtcClock.h"

RtcClock::RtcClock(uint32_t refreshMs) : _refreshMs(refreshMs) {}

bool RtcClock::begin() {
    _ok = _rtc.begin();
    if (!_ok) {
        Serial.println("[RTC] FAIL: DS1307 not found (expected 0x68)");
        return false;
    }

    _running = _rtc.isrunning();
    Serial.printf("[RTC] OK | running=%d\n", _running ? 1 : 0);

    if (!_running) {
        Serial.println("[RTC] not running -> adjust to compile time");
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        delay(10);
        _running = _rtc.isrunning();
        Serial.printf("[RTC] running(after adjust)=%d\n", _running ? 1 : 0);
    }

    _now = _rtc.now();
    _lastMs = millis();
    Serial.printf("[RTC] now %02d:%02d:%02d\n", _now.hour(), _now.minute(), _now.second());
    return true;
}

void RtcClock::poll() {
    if (!_ok) return;

    const uint32_t nowMs = millis();
    if (nowMs - _lastMs < _refreshMs) return;
    _lastMs = nowMs;

    _now = _rtc.now();
    _running = _rtc.isrunning();
}

RtcClock::Snapshot RtcClock::snapshot() const {
    Snapshot s;
    s.ok = _ok;
    s.running = _running;
    s.now = _now;
    return s;
}

void RtcClock::formatTimeOrUptime(char* out, size_t outSz) const {
    if (_ok && _running) {
        snprintf(out, outSz, "%02d:%02d:%02d",
                 _now.hour(), _now.minute(), _now.second());
    } else {
        const uint32_t up = millis() / 1000;
        snprintf(out, outSz, "up %lus", (unsigned long)up);
    }
}