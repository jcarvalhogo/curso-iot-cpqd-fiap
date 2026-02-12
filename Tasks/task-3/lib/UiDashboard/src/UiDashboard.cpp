//
// Created by Josemar Carvalho on 11/02/26.
//

#include "UiDashboard.h"
#include <math.h>

UiDashboard::UiDashboard(OledSh1107& oled, const RtcClock& rtc, uint32_t refreshMs)
    : _oled(oled), _rtc(rtc), _refreshMs(refreshMs) {}

void UiDashboard::setRefreshMs(uint32_t refreshMs) {
    _refreshMs = refreshMs;
}

void UiDashboard::formatPpm(char* out, size_t outSz, float ppm) {
    if (!isfinite(ppm)) { snprintf(out, outSz, "--"); return; }
    if (ppm >= 100000.0f) snprintf(out, outSz, "100K");
    else if (ppm >= 1000.0f) snprintf(out, outSz, "%.1fK", ppm / 1000.0f);
    else if (ppm >= 100.0f) snprintf(out, outSz, "%.0f", ppm);
    else snprintf(out, outSz, "%.1f", ppm);
}

void UiDashboard::tick(const Inputs& in) {
    const uint32_t now = millis();
    if (now - _lastMs < _refreshMs) return;
    _lastMs = now;
    draw(in);
}

void UiDashboard::draw(const Inputs& in) {
    char l1[32], l2[32], l3[32];

    char tbuf[16];
    _rtc.formatTimeOrUptime(tbuf, sizeof(tbuf));

    snprintf(l1, sizeof(l1), "%s | MQ2 %s",
             tbuf,
             (in.mq2SimMode ? "SIM" : (in.mq2Calibrated ? "R0 ok" : "calib!")));

    if (in.dht.ok) {
        snprintf(l2, sizeof(l2), "T:%.1fC H:%.1f%%", in.dht.temperatureC, in.dht.humidity);
    } else {
        snprintf(l2, sizeof(l2), "DHT: ERRO (%lu)", (unsigned long)in.dhtFailCount);
    }

    const bool showLpg = ((millis() / _refreshMs) % 2) == 1;

    if (isfinite(in.smokePpm) || isfinite(in.lpgPpm)) {
        char ppmTxt[12];
        if (showLpg) {
            formatPpm(ppmTxt, sizeof(ppmTxt), in.lpgPpm);
            snprintf(l3, sizeof(l3), "LPG:%s N:%.2f", ppmTxt, in.mq2.normalized);
        } else {
            formatPpm(ppmTxt, sizeof(ppmTxt), in.smokePpm);
            snprintf(l3, sizeof(l3), "Smk:%s N:%.2f", ppmTxt, in.mq2.normalized);
        }
    } else {
        snprintf(l3, sizeof(l3), "MQ2 N:%.2f A:%u", in.mq2.normalized, (unsigned)in.mq2.adc);
    }

    _oled.drawStatus3(l1, l2, l3);
}
