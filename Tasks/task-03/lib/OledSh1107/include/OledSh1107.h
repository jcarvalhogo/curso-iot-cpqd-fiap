//
// Created by Josemar Carvalho on 11/02/26.
//

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

class OledSh1107 {
public:
    // Defaults comuns para Grove SH1107 via I2C
    explicit OledSh1107(uint8_t sda = 8, uint8_t scl = 9, uint8_t addr = 0x3C);

    bool begin();

    void clear();

    void setContrast(uint8_t value); // 0..255

    // Helpers de texto (simples e práticos)
    void drawTitle(const char *title);

    void drawStatus2(const char *line1, const char *line2);

    void drawStatus3(const char *line1, const char *line2, const char *line3);

    // Debug rápido
    void drawBootScreen(const char *appName);

private:
    void drawCentered(uint8_t y, const char *text);

private:
    uint8_t _sda;
    uint8_t _scl;
    uint8_t _addr;

    // Full framebuffer (F) — melhor para desenhos/atualização completa
    U8G2_SH1107_128X128_F_HW_I2C _u8g2;

    bool _started = false;
};
