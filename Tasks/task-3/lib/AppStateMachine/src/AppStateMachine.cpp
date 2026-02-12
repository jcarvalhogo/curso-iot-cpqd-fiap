//
// Created by Josemar Carvalho on 11/02/26.
//

#include "AppStateMachine.h"
#include <math.h>

AppStateMachine::AppStateMachine(LedRgbStatus& led) : _led(led) {}

void AppStateMachine::set(State s) {
    if (_st == s) return;
    _st = s;

    switch (_st) {
        case State::Boot:        _led.statusBoot(); break;
        case State::Running:     _led.statusOnline(); break;
        case State::SensorError: _led.statusError(); break;
        case State::GasWarn:     _led.statusWarn(); break;
        case State::CalibFail:   _led.statusError(); break;
    }
}

void AppStateMachine::setBoot() { set(State::Boot); }

void AppStateMachine::evaluate(bool mq2SimMode,
                               bool mq2Calibrated,
                               bool dhtOk,
                               float smokePpm,
                               float enterPpm,
                               float exitPpm) {
    if (!mq2SimMode && !mq2Calibrated) { set(State::CalibFail); return; }
    if (!dhtOk) { set(State::SensorError); return; }

    if (isfinite(smokePpm) && smokePpm > enterPpm) { set(State::GasWarn); return; }
    if (_st == State::GasWarn && isfinite(smokePpm) && smokePpm < exitPpm) { set(State::Running); return; }

    set(State::Running);
}
