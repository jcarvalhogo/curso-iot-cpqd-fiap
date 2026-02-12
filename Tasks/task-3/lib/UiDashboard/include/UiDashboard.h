//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_3_UIDASHBOARD_H
#define TASK_3_UIDASHBOARD_H

#pragma once
#include <Arduino.h>
#include "OledSh1107.h"
#include "Dht22Sensor.h"
#include "Mq2GasSensor.h"
#include "RtcClock.h"

class UiDashboard {
public:
    struct Inputs {
        bool mq2SimMode{true};
        bool mq2Calibrated{false};

        Dht22Sensor::Reading dht{};
        Mq2GasSensor::Reading mq2{};
        float smokePpm{NAN};
        float lpgPpm{NAN};

        uint32_t dhtFailCount{0};
    };

    UiDashboard(OledSh1107& oled, const RtcClock& rtc, uint32_t refreshMs = 800);

    void setRefreshMs(uint32_t refreshMs);
    void tick(const Inputs& in); // desenha no OLED respeitando refresh

private:
    static void formatPpm(char* out, size_t outSz, float ppm);

    void draw(const Inputs& in);

private:
    OledSh1107& _oled;
    const RtcClock& _rtc;
    uint32_t _refreshMs{800};
    uint32_t _lastMs{0};
};


#endif //TASK_3_UIDASHBOARD_H