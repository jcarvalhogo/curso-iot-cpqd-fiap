//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_3_RTCCLOCK_H
#define TASK_3_RTCCLOCK_H

#pragma once

#include <Arduino.h>
#include <RTClib.h>

class RtcClock {
public:
    struct Snapshot {
        bool ok{false};
        bool running{false};
        DateTime now{};
    };

    explicit RtcClock(uint32_t refreshMs = 500);

    bool begin();                 // chama rtc.begin(), adjust se precisar
    void poll();                  // atualiza agora respeitando refreshMs
    Snapshot snapshot() const;    // estado atual

    void formatTimeOrUptime(char* out, size_t outSz) const;

private:
    RTC_DS1307 _rtc;
    bool _ok{false};
    bool _running{false};
    DateTime _now{};
    uint32_t _refreshMs{500};
    uint32_t _lastMs{0};
};

#endif //TASK_3_RTCCLOCK_H
