#include <Arduino.h>
#include "LedRgbStatus.h"

#ifndef RGB_STATUS_PIN
#define RGB_STATUS_PIN 38
#endif

#ifndef RGB_STATUS_BRIGHTNESS
#define RGB_STATUS_BRIGHTNESS 32
#endif

LedRgbStatus led(RGB_STATUS_PIN, RGB_STATUS_BRIGHTNESS);

enum class SystemState {
    Boot,
    Connecting,
    Online,
    Error
};

SystemState currentState = SystemState::Boot;
uint32_t stateStart = 0;

void setState(SystemState newState) {
    currentState = newState;
    stateStart = millis();

    switch (currentState) {
        case SystemState::Boot:
            Serial.println("[SYS] Booting...");
            led.statusBoot();
            break;

        case SystemState::Connecting:
            Serial.println("[SYS] Connecting WiFi...");
            led.statusWifiConnecting();
            break;

        case SystemState::Online:
            Serial.println("[SYS] Online.");
            led.statusOnline();
            break;

        case SystemState::Error:
            Serial.println("[SYS] ERROR!");
            led.statusError();
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    led.begin();

    setState(SystemState::Boot);
}

void loop() {
    led.update();

    uint32_t now = millis();

    switch (currentState) {
        case SystemState::Boot:
            if (now - stateStart > 2000) {
                setState(SystemState::Connecting);
            }
            break;

        case SystemState::Connecting:
            if (now - stateStart > 4000) {
                setState(SystemState::Online);
            }
            break;

        case SystemState::Online:
            // Aqui ficaria normal até acontecer erro
            break;

        case SystemState::Error:
            break;
    }
}
