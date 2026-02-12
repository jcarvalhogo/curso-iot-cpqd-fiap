//
// Created by Josemar Carvalho on 11/02/26.
//
#include "AppController.h"

static constexpr uint8_t I2C_SDA = 21;
static constexpr uint8_t I2C_SCL = 22;
static constexpr uint8_t OLED_ADDR = 0x3C;

static constexpr uint8_t DHT_PIN = 4;
static constexpr uint8_t MQ2_PIN = 34;

static constexpr uint32_t UI_REFRESH_MS = 800;
static constexpr uint32_t WIFI_POLL_MS  = 250;

AppController::AppController()
    : _led(),
      _oled(I2C_SDA, I2C_SCL, OLED_ADDR),
      _dht(DHT_PIN, 2000),
      _mq2(MQ2_PIN),
      _rtc(500),
      _wifi() {
}

void AppController::begin() {
    Serial.begin(115200);
    delay(200);

    _led.begin();

    // I2C uma vez só
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);

    _oledOk = _oled.begin();
    if (_oledOk) _oled.drawStatus3("Boot", "Init...", "WiFi/DHT/MQ2/RTC");

    _rtc.begin();
    _dht.begin();
    _mq2.begin();

    // -----------------------------
    // ✅ WiFiManager Config (sem brace-init)
    // -----------------------------
    _wifiCfg = WiFiManager::Config();  // explícito (opcional)

    // ⬇️⬇️⬇️ AJUSTE AQUI conforme os campos reais do seu Config
    //
    // Exemplos comuns:
    // _wifiCfg.ssid = "Wokwi-GUEST";
    // _wifiCfg.password = "";
    // _wifiCfg.connectTimeoutMs = 15000;
    // _wifiCfg.retryDelayMs = 500;
    // _wifiCfg.maxRetries = 30;
    // _wifiCfg.autoReconnect = true;
    //
    // Se seu WiFiManager tiver setters, use setters.

    // Exemplo mínimo: se o seu WiFiManager tiver begin(cfg)
    // _wifi.begin(_wifiCfg);

    // Ou se ele tiver setConfig(cfg) + begin()
    // _wifi.setConfig(_wifiCfg);
    // _wifi.begin();

    // Como eu não vejo a API real agora, deixei as duas opções comentadas.
    // ✅ Assim que você colar o trecho do Config/API eu fecho isso 100% compilável.

    _lastUiMs = millis();
    _lastWifiMs = millis();
    updateUi();
}

void AppController::loop() {
    _led.update();

    // RTC
    _rtc.poll();

    // WiFi (poll leve)
    const uint32_t now = millis();
    if (now - _lastWifiMs >= WIFI_POLL_MS) {
        _lastWifiMs = now;

        // exemplo:
        // _wifi.loop();  // ou _wifi.poll(); depende da sua lib
    }

    // DHT (respeita o gate interno)
    if (_dht.read()) {
        _lastDht = _dht.last();
    }

    // MQ2 (rápido)
    _lastMq2 = _mq2.read(10, 2);

    updateState();

    if (now - _lastUiMs >= UI_REFRESH_MS) {
        _lastUiMs = now;
        updateUi();
    }
}

void AppController::updateState() {
    // aqui você pluga sua state-machine (Boot/Running/GasWarn/etc.)
    // e usa _led.statusOnline(), _led.statusWarn(), etc.
}

void AppController::updateUi() {
    if (!_oledOk) return;

    char tbuf[16];
    _rtc.formatTimeOrUptime(tbuf, sizeof(tbuf));

    snprintf(_l1, sizeof(_l1), "%s | WiFi", tbuf);

    if (_lastDht.ok) {
        snprintf(_l2, sizeof(_l2), "T:%.1fC H:%.1f%%", _lastDht.temperatureC, _lastDht.humidity);
    } else {
        snprintf(_l2, sizeof(_l2), "DHT: ERRO");
    }

    snprintf(_l3, sizeof(_l3), "MQ2 N:%.2f A:%u", _lastMq2.normalized, (unsigned)_lastMq2.adc);

    _oled.drawStatus3(_l1, _l2, _l3);
}
