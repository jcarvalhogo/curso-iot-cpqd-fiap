//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_3_APPCONTROLLER_H
#define TASK_3_APPCONTROLLER_H

#pragma once

#include <Arduino.h>

#include "LedRgbStatus.h"
#include "OledSh1107.h"
#include "Dht22Sensor.h"
#include "Mq2GasSensor.h"
#include "RtcClock.h"

// ✅ vem do shared-libs
#include "WiFiManager.h"

class AppController {
public:
    AppController();

    void begin();
    void loop();

private:
    void updateUi();
    void updateState();

private:
    // --- devices/libs ---
    LedRgbStatus _led;
    OledSh1107   _oled;
    Dht22Sensor  _dht;
    Mq2GasSensor _mq2;
    RtcClock     _rtc;

    // ✅ aqui é o ponto do erro: NÃO inicialize Config com { ... }
    WiFiManager _wifi;
    WiFiManager::Config _wifiCfg;   // configura depois no .cpp

    bool _oledOk{false};

    // --- timings ---
    uint32_t _lastUiMs{0};
    uint32_t _lastWifiMs{0};

    // --- cached readings ---
    Dht22Sensor::Reading _lastDht{false, NAN, NAN, 0};
    Mq2GasSensor::Reading _lastMq2{false, 0, 0.0f, 0.0f, 0};

    // --- ui buffers ---
    char _l1[32]{};
    char _l2[32]{};
    char _l3[32]{};
};

#endif //TASK_3_APPCONTROLLER_H