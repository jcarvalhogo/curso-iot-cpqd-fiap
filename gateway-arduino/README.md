# Gateway Arduino – IoT Secure Gateway

## Visão Geral
O **gateway-arduino** é um gateway IoT embarcado baseado em **ESP32**, responsável por receber telemetria segura de dispositivos IoT, validar autenticidade e integridade das mensagens e encaminhar os dados para plataformas de nuvem como **Ubidots** e **ThingSpeak**.

O projeto atua como um **ponto intermediário confiável**, isolando dispositivos embarcados da exposição direta à Internet e concentrando as regras de segurança, rate-limit e integração externa.

---

## Arquitetura Geral

```
+-------------------+        HTTP Secure (AES-GCM + HMAC)        +----------------------+
|  vehicle-device   |  --------------------------------------> |   gateway-arduino    |
|  (ESP32)          |                                           |   (ESP32)            |
|                   |                                           |                      |
| - Sensores        |                                           | - SecureHttp Auth    |
| - SecureDeviceAuth|                                           | - HTTP Server        |
+-------------------+                                           | - Ubidots Client     |
                                                                | - ThingSpeak Client  |
                                                                +----------+-----------+
                                                                           |
                                    +--------------------------------------+---------------------+
                                    |                                                            |
                              +-----v-----+                                               +------v------+
                              | Ubidots   |                                               | ThingSpeak  |
                              | (MQTT)    |                                               | (HTTP REST) |
                              +-----------+                                               +-------------+
```

---

## Comunicação e Segurança

### SecureHttp
A comunicação entre device e gateway utiliza a biblioteca **SecureHttp**, que implementa múltiplas camadas de segurança:

### Mecanismos Implementados

| Mecanismo | Descrição |
|---------|----------|
| AES-256-GCM | Criptografia simétrica autenticada (confidencialidade + integridade) |
| HMAC-SHA256 | Autenticação do dispositivo |
| Timestamp (epoch) | Proteção contra replay |
| Nonce | Identificador único por requisição |
| AAD | Proteção dos metadados da requisição |

### Canonical String para Assinatura

```
METHOD
PATH
DEVICE_ID
TIMESTAMP
NONCE
IV
TAG
CIPHERTEXT_HEX
```

---

## Sincronização de Tempo (NTP)

O gateway utiliza **NTP** para garantir validade temporal das mensagens:

- Sincroniza o relógio sempre que o Wi-Fi conecta
- Rejeita mensagens se o epoch não estiver válido
- Evita falsos positivos de replay

---

## Componentes do Sistema

### WiFiManager
- Gerenciamento de conexão Wi-Fi
- Reconexão automática
- Configuração de hostname e potência TX

### HttpServer
- Servidor HTTP local (porta 8045)
- Endpoint principal: `POST /telemetry`
- Validação SecureHttp
- Orquestra callbacks internos

### SecureGatewayAuth
- Validação HMAC
- Checagem de timestamp e nonce
- Decriptação AES-GCM
- Bloqueio de mensagens inválidas

### UbidotsClient
- Envio imediato de telemetria
- Comunicação MQTT
- Baixa latência

### ThingSpeakClient
- Envio periódico via HTTP REST
- Rate-limit controlado por timer

### LedStatus
- Indicação visual do estado do gateway
- Estados de boot, conexão e falha

---

## Fluxo de Execução

### Inicialização
1. Boot do ESP32
2. LED em BLINK_FAST
3. Conexão Wi-Fi
4. Sincronização NTP
5. Inicialização do servidor HTTP
6. Inicialização dos clientes de nuvem

### Recepção de Telemetria
1. Device envia POST /telemetry
2. SecureHttp valida e decripta
3. Dados logados localmente
4. Envio imediato para Ubidots
5. Envio periódico para ThingSpeak

---

## Configuração

### secrets.h
```cpp
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"

#define UBIDOTS_TOKEN "your-ubidots-token"
#define UBIDOTS_DEVICE_LABEL "gateway-device"

#define THINGSPEAK_WRITE_KEY "your-thingspeak-key"
```

---

## Porta e Endpoint
- Porta HTTP: **8045**
- Endpoint: `/telemetry`

---

## Logs e Diagnóstico
Exemplo:
```
[WiFi] Connected
[Time] Gateway NTP synced
[TEL] T=27.10 H=74.30 fuel=70 speed=40.9 rpm=3273
[Ubidots] Telemetry sent
```

Erros comuns:
- `timestamp_out_of_window`
- `bad_signature`
- `decrypt_failed`

---

## Requisitos
- ESP32
- Arduino Framework
- PlatformIO
- Wi-Fi com acesso à Internet
- Conta Ubidots
- Conta ThingSpeak

---

## Considerações de Segurança
- Nunca versionar `secrets.h`
- Usar chaves longas e aleatórias
- Gateway é ponto central de confiança

---

## Possíveis Evoluções
- HTTPS/TLS no servidor local
- Suporte a múltiplos devices
- Buffer offline
- Integração com MQTT broker local

---

## Licença
Projeto educacional / acadêmico – FIAP / CPQD
