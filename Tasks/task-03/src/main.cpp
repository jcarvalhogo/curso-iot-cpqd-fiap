#include <Arduino.h>
#include "LedRgbStatus.h"

#define RGB_PIN 48   // <-- coloque o GPIO que funcionou no seu scan!

LedRgbStatus rgb(RGB_PIN, 32);

void setup() {
    Serial.begin(115200);
    rgb.begin();

    rgb.statusBoot();
    delay(1000);

    rgb.statusWifiConnecting();
}

void loop() {
    rgb.update();

    static uint32_t t0 = millis();
    if (millis() - t0 > 6000) {
        rgb.statusOnline();
    }
}
