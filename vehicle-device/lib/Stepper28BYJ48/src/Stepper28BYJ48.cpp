//
// Created by Josemar Carvalho on 09/02/26.
//
#include "Stepper28BYJ48.h"

Stepper28BYJ48::Stepper28BYJ48(const Pins &pins, const SpeedPotConfig &pot, const MotionConfig &motion)
    : _pins(pins),
      _pot(pot),
      _motion(motion),
      // HALF4WIRE e ordem recomendada pro 28BYJ-48 (IN1, IN3, IN2, IN4)
      _stepper(AccelStepper::HALF4WIRE, pins.in1, pins.in3, pins.in2, pins.in4) {
}

void Stepper28BYJ48::begin() {
    pinMode(_pot.adcPin, INPUT);

    // resolução do ADC no ESP32
    analogReadResolution(12); // 0..4095

    _stepper.setMaxSpeed(_motion.maxSpeed);
    _speedRaw = analogRead(_pot.adcPin);
    _speed = mapSpeed(_speedRaw);
    _stepper.setSpeed(_speed);
}

int Stepper28BYJ48::readAdcAveraged(uint8_t pin, uint8_t samples) const {
    if (samples == 0) samples = 1;
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(2);
    }
    return (int) (sum / samples);
}

float Stepper28BYJ48::mapSpeed(int adcRaw) const {
    adcRaw = constrain(adcRaw, 0, 4095);
    float t = (float) adcRaw / 4095.0f;
    if (_pot.invert) t = 1.0f - t;
    return _motion.minSpeed + t * (_motion.maxSpeed - _motion.minSpeed);
}

void Stepper28BYJ48::update() {
    const uint32_t now = millis();

    if (now - _lastUpdateMs >= _pot.updateMs) {
        _lastUpdateMs = now;
        _speedRaw = readAdcAveraged(_pot.adcPin, _pot.samples);
        _speed = mapSpeed(_speedRaw);
        _stepper.setSpeed(_speed);
    }

    // não-bloqueante: mantém o motor girando
    _stepper.runSpeed();
}
