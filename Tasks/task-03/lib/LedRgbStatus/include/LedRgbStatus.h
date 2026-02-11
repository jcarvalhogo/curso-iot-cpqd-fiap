//
// Created by Josemar Carvalho on 11/02/26.
//

#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Permite configurar via build_flags (PlatformIO)
#ifndef RGB_STATUS_PIN
#define RGB_STATUS_PIN 38
#endif

#ifndef RGB_STATUS_BRIGHTNESS
#define RGB_STATUS_BRIGHTNESS 32
#endif

// Por padrão GRB (no seu DevKitC-1 funcionou com GRB)
#ifndef RGB_STATUS_COLOR_ORDER
#define RGB_STATUS_COLOR_ORDER NEO_GRB
#endif

// Permite trocar a frequência se precisar (normal WS2812 = 800kHz)
#ifndef RGB_STATUS_SIGNAL_FREQ
#define RGB_STATUS_SIGNAL_FREQ NEO_KHZ800
#endif

class LedRgbStatus {
public:
    enum class Mode : uint8_t {
        Off,
        Solid,
        Blink,
        Pulse
    };

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

    explicit LedRgbStatus(uint8_t pin = RGB_STATUS_PIN,
                          uint8_t brightness = RGB_STATUS_BRIGHTNESS,
                          uint16_t pixelsCount = 1);

    void begin();

    void setBrightness(uint8_t brightness);

    // Controle direto
    void setOff();

    void setSolid(Color c);

    void setBlink(Color c, uint16_t onMs = 200, uint16_t offMs = 200);

    void setPulse(Color c, uint16_t periodMs = 1200);

    // Presets (IoT)
    void statusBoot(); // azul piscando
    void statusWifiConnecting(); // amarelo pulsando
    void statusOnline(); // verde fixo
    void statusWarn(); // ciano piscando
    void statusError(); // vermelho piscando rápido

    // Chame no loop()
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

    // blink
    uint16_t _onMs = 200;
    uint16_t _offMs = 200;
    bool _blinkOn = false;

    // pulse
    uint16_t _periodMs = 1200;

    // timing
    uint32_t _lastMs = 0;
};
