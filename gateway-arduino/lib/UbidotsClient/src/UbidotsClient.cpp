//
// Created by Josemar Carvalho on 07/02/26.
//

#include "UbidotsClient.h"

UbidotsClient::UbidotsClient(const Config &cfg)
    : _cfg(cfg), _mqtt(_net) {
}

void UbidotsClient::begin() {
    _mqtt.setServer(_cfg.host, _cfg.port);
}

void UbidotsClient::update() {
    if (WiFi.status() != WL_CONNECTED) return;

    ensureConnected();
    _mqtt.loop();
}

bool UbidotsClient::isConnected() {
    return _mqtt.connected();
}

bool UbidotsClient::ensureConnected() {
    if (_mqtt.connected()) return true;

    const uint32_t now = millis();
    if (now - _lastReconnectAttempt < _cfg.reconnectIntervalMs) return false;
    _lastReconnectAttempt = now;

    if (!_cfg.token || !_cfg.deviceLabel) {
        Serial.println("[Ubidots] Missing token/deviceLabel");
        return false;
    }

    Serial.print("[Ubidots] Connecting to MQTT ");
    Serial.print(_cfg.host);
    Serial.print(":");
    Serial.println(_cfg.port);

    // Ubidots auth: username=TOKEN, password=TOKEN
    const bool ok = _mqtt.connect(_cfg.clientId, _cfg.token, _cfg.token);
    if (ok) {
        Serial.println("[Ubidots] MQTT connected");
    } else {
        Serial.print("[Ubidots] MQTT connect failed, rc=");
        Serial.println(_mqtt.state());
    }
    return ok;
}

String UbidotsClient::makeTopic() const {
    // Ubidots publish topic: /v1.6/devices/<device_label>
    String topic = "/v1.6/devices/";
    topic += _cfg.deviceLabel;
    return topic;
}

String UbidotsClient::makePayload(float temperature, float humidity) const {
    // Minimal JSON payload
    String s = "{";
    bool first = true;

    if (!isnan(temperature)) {
        s += "\"temperature\":";
        s += String(temperature, 2);
        first = false;
    }
    if (!isnan(humidity)) {
        if (!first) s += ",";
        s += "\"humidity\":";
        s += String(humidity, 2);
    }

    s += "}";
    return s;
}

bool UbidotsClient::publishTelemetry(float temperature, float humidity) {
    if (WiFi.status() != WL_CONNECTED) return false;
    if (!ensureConnected()) return false;

    const String topic = makeTopic();
    const String payload = makePayload(temperature, humidity);

    Serial.print("[Ubidots] PUB ");
    Serial.print(topic);
    Serial.print(" ");
    Serial.println(payload);

    return _mqtt.publish(topic.c_str(), payload.c_str());
}
