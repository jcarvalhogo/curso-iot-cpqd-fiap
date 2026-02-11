#include <Arduino.h>
#include "OledSh1107.h"
#include "LedRgbStatus.h"

// Ajuste conforme seus pinos I2C reais no ESP32-S3
static constexpr uint8_t I2C_SDA = 8;
static constexpr uint8_t I2C_SCL = 9;
static constexpr uint8_t OLED_ADDR = 0x3C;

OledSh1107 oled(I2C_SDA, I2C_SCL, OLED_ADDR);
LedRgbStatus led; // usa defaults via build_flags (pin 38 etc.)

enum class State : uint8_t { Boot, Connecting, Online, Error };

static State st = State::Boot;
static uint32_t t0 = 0;
static bool oledOk = false;

static void setState(State s) {
    st = s;
    t0 = millis();

    switch (st) {
        case State::Boot:
            Serial.println("[SYS] Boot...");
            led.statusBoot();
            if (oledOk) oled.drawBootScreen("ESP32-S3");
            break;

        case State::Connecting:
            Serial.println("[SYS] WiFi connecting...");
            led.statusWifiConnecting();
            if (oledOk) oled.drawStatus3("WiFi:", "Conectando...", "Aguarde");
            break;

        case State::Online:
            Serial.println("[SYS] Online.");
            led.statusOnline();
            if (oledOk) oled.drawStatus3("WiFi:", "Conectado", "IP: 192.168.0.10");
            break;

        case State::Error:
            Serial.println("[SYS] ERROR!");
            led.statusError();
            if (oledOk) oled.drawStatus3("Sistema:", "ERRO", "Verifique logs");
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(400);

    Serial.println("\n[APP] Starting...");

    led.begin();

    oledOk = oled.begin();
    Serial.printf("[OLED] begin: %s (SDA=%u SCL=%u addr=0x%02X)\n",
                  oledOk ? "OK" : "FAIL", I2C_SDA, I2C_SCL, OLED_ADDR);

    setState(State::Boot);
}

void loop() {
    led.update();

    const uint32_t now = millis();

    switch (st) {
        case State::Boot:
            if (now - t0 > 1500) setState(State::Connecting);
            break;

        case State::Connecting:
            // Simulação: após 4s "conectou"
            if (now - t0 > 4000) setState(State::Online);
            break;

        case State::Online:
            // aqui ficaria rodando a aplicação real
            break;

        case State::Error:
            break;
    }
}
