//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_3_APPSTATEMACHINE_H
#define TASK_3_APPSTATEMACHINE_H

#pragma once
#include <Arduino.h>
#include "LedRgbStatus.h"

class AppStateMachine {
public:
    enum class State : uint8_t { Boot, Running, SensorError, GasWarn, CalibFail };

    explicit AppStateMachine(LedRgbStatus& led);

    void setBoot();
    void evaluate(bool mq2SimMode,
                  bool mq2Calibrated,
                  bool dhtOk,
                  float smokePpm,
                  float enterPpm,
                  float exitPpm);

    State state() const { return _st; }

private:
    void set(State s);

private:
    LedRgbStatus& _led;
    State _st{State::Boot};
};


#endif //TASK_3_APPSTATEMACHINE_H