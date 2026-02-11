//
// Created by Josemar Carvalho on 11/02/26.
//

#include "OledSh1107.h"

OledSh1107::OledSh1107(uint8_t sda, uint8_t scl, uint8_t addr)
    : _sda(sda),
      _scl(scl),
      _addr(addr),
      _u8g2(U8G2_R0, U8X8_PIN_NONE) {
}

bool OledSh1107::begin() {
    Wire.begin(_sda, _scl);

    _u8g2.setI2CAddress((uint8_t) (_addr << 1));
    _u8g2.begin();

    _u8g2.setFont(u8g2_font_6x13_tf);
    _u8g2.setFontMode(1);
    _u8g2.setDrawColor(1);

    _started = true;
    clear();
    return true;
}

void OledSh1107::clear() {
    if (!_started) return;
    _u8g2.clearBuffer();
    _u8g2.sendBuffer();
}

void OledSh1107::setContrast(uint8_t value) {
    if (!_started) return;
    _u8g2.setContrast(value);
}

void OledSh1107::drawCentered(uint8_t y, const char *text) {
    if (!_started || !text) return;
    int16_t w = _u8g2.getStrWidth(text);
    int16_t x = (128 - w) / 2;
    if (x < 0) x = 0;
    _u8g2.drawStr(x, y, text);
}

void OledSh1107::drawTitle(const char *title) {
    if (!_started) return;
    _u8g2.clearBuffer();
    drawCentered(16, title ? title : "");
    _u8g2.drawHLine(0, 22, 128);
    _u8g2.sendBuffer();
}

void OledSh1107::drawStatus2(const char *line1, const char *line2) {
    if (!_started) return;
    _u8g2.clearBuffer();
    _u8g2.drawStr(0, 20, line1 ? line1 : "");
    _u8g2.drawStr(0, 40, line2 ? line2 : "");
    _u8g2.sendBuffer();
}

void OledSh1107::drawStatus3(const char *line1, const char *line2, const char *line3) {
    if (!_started) return;
    _u8g2.clearBuffer();
    _u8g2.drawStr(0, 20, line1 ? line1 : "");
    _u8g2.drawStr(0, 40, line2 ? line2 : "");
    _u8g2.drawStr(0, 60, line3 ? line3 : "");
    _u8g2.sendBuffer();
}

void OledSh1107::drawBootScreen(const char *appName) {
    if (!_started) return;
    _u8g2.clearBuffer();
    drawCentered(28, appName ? appName : "APP");
    drawCentered(52, "BOOT...");
    _u8g2.drawFrame(14, 70, 100, 10);
    _u8g2.drawBox(16, 72, 30, 6);
    _u8g2.sendBuffer();
}
