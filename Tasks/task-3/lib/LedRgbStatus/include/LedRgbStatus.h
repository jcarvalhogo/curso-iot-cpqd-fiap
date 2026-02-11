//
// Created by Josemar Carvalho on 11/02/26.
//

#ifndef TASK_03_LEDRGBSTATUS_H
#define TASK_03_LEDRGBSTATUS_H

//
// Created by Josemar Carvalho on 11/02/26.
//

#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LedRgbStatus {
public:
    enum class Mode : uint8_t { Off, Solid, Blink, Pulse };

    struct Color {
        uint8_t r, g, b;
        static constexpr Color Off() { return {0, 0, 0}; }
        static constexpr Color Red() { return {255, 0, 0}; }
        static constexpr Color Green() { return {0, 255, 0}; }
        static constexpr Color Blue() { return {0, 0, 255}; }
        static constexpr Color Yellow() { return {255, 255, 0}; }
        static constexpr Color Cyan() { return {0, 255, 255}; }
        static constexpr Color Purple() { return {255, 0, 255}; }
        static constexpr Color White() { return {255, 255, 255}; }
    };

    // Defaults internos: GPIO 5 (ESP32 clássico), brilho 32
    explicit LedRgbStatus(uint8_t pin = 5,
                          uint8_t brightness = 32,
                          uint16_t pixelsCount = 1,
                          neoPixelType type = NEO_GRB + NEO_KHZ800);

    void begin();

    void setBrightness(uint8_t brightness);

    void setOff();

    void setSolid(Color c);

    void setBlink(Color c, uint16_t onMs = 200, uint16_t offMs = 200);

    void setPulse(Color c, uint16_t periodMs = 1200);

    // Presets IoT
    void statusBoot();

    void statusWifiConnecting();

    void statusOnline();

    void statusWarn();

    void statusError();

    void update();

private:
    void showColor(uint8_t r, uint8_t g, uint8_t b);

private:
    uint8_t _pin;
    uint8_t _brightness;
    uint16_t _pixelsCount;

    Adafruit_NeoPixel _strip;

    Mode _mode = Mode::Off;
    Color _color = Color::Off();

    uint16_t _onMs = 200;
    uint16_t _offMs = 200;
    bool _blinkOn = false;

    uint16_t _periodMs = 1200;
    uint32_t _lastMs = 0;
};
#endif //TASK_03_LEDRGBSTATUS_H
