# vehicle-device

## Visão Geral
O **vehicle-device** é um firmware embarcado para ESP32 que simula e coleta dados de um veículo (ou equipamento móvel),
agregando sensores físicos e sinais analógicos e enviando telemetria de forma segura para o **gateway-arduino** via HTTP,
utilizando a biblioteca **SecureHttp**.

O projeto foi desenvolvido no contexto do curso IoT (CPQD/FIAP) e tem como foco:
- Integração de sensores
- Simulação de sinais automotivos
- Comunicação segura dispositivo → gateway
- Arquitetura modular com bibliotecas reutilizáveis

---

## Arquitetura do Sistema

```
+-------------------+
| vehicle-device    |
| (ESP32)           |
|                   |
|  - DHT22          |
|  - Fuel ADC       |
|  - Accel ADC      |
|                   |
|  SecureHttp       |
+---------+---------+
          |
          | HTTPS-like (HTTP + HMAC + AES-GCM)
          |
+---------v---------+
| gateway-arduino   |
| (ESP32)           |
+-------------------+
```

---

## Sensores e Sinais

### 🌡️ DHT22
- Temperatura (°C)
- Umidade relativa (%)
- Biblioteca: `DhtSensor`
- Intervalo mínimo configurável (default: 2s)

### ⛽ Fuel Level (ADC)
- Leitura analógica (GPIO 34)
- Conversão para porcentagem
- Calibração via `adcMin` / `adcMax`
- Biblioteca: `FuelLevel`

### 🚗 Aceleração (Simulada)
- Potenciômetro no ADC (GPIO 35)
- Curva não-linear (gamma)
- Suavização por EMA (Exponential Moving Average)
- Conversão para:
  - Aceleração (%)
  - RPM simulado (0–8000)

---

## Comunicação com Gateway

### Protocolo
- HTTP POST `/telemetry`
- Payload criptografado (AES-256-GCM)
- Autenticação e integridade via HMAC-SHA256
- Proteção contra replay (timestamp + nonce)

### Biblioteca
- `GatewayClient`
- Usa internamente `SecureDeviceAuth`

### Campos enviados
- `temperature`
- `humidity`
- `fuelLevel`
- `stepperSpeed` → aceleração (%)
- `stepperRpm` → RPM simulado

---

## Segurança (SecureHttp)

A comunicação é protegida por:

1. **AES-256-GCM**
   - Confidencialidade
   - Integridade (TAG)

2. **HMAC-SHA256**
   - Assinatura do payload canônico

3. **Anti-replay**
   - Timestamp (epoch)
   - Nonce único por requisição

4. **Device ID**
   - Header `X-Device-Id`

### Fluxo resumido
1. Device gera timestamp e nonce
2. Criptografa JSON
3. Assina canonical string
4. Envia headers + body
5. Gateway valida assinatura, timestamp e nonce
6. Gateway decripta payload

---

## Wi-Fi

- Biblioteca: `WiFiManager` (custom)
- Conexão automática com SSID/SENHA definidos em `secrets.h`
- Reconexão automática
- Configuração de potência TX e sleep

---

## Estrutura do Projeto

```
vehicle-device/
├── src/
│   └── main.cpp
├── lib/
│   ├── DhtSensor/
│   ├── FuelLevel/
│   ├── GatewayClient/
│   ├── SecureHttp/
│   ├── WiFiManager/
├── include/
├── platformio.ini
└── README.md
```

---

## Configuração

### secrets.h
```cpp
#define WIFI_SSID "..."
#define WIFI_PASS "..."
```

### Gateway
```cpp
#define GATEWAY_HOST "192.168.3.12"
#define GATEWAY_PORT 8045
```

---

## Loop Principal

1. Atualiza Wi-Fi
2. Atualiza sensores
3. Calcula aceleração e RPM
4. Log periódico via Serial
5. Envia telemetria respeitando rate-limit

---

## Logs de Exemplo

```
[DHT] T=27.10 C | H=74.30 %
[Fuel] raw=2751 | level=70 %
[Accel] raw=2717 | accel=40.9 % | rpm(sim)=3273
[Gateway] Telemetry sent
```

---

## Objetivo Didático

Este projeto demonstra:
- Boas práticas em firmware ESP32
- Comunicação segura sem TLS
- Arquitetura orientada a bibliotecas
- Integração IoT Device → Gateway → Cloud

---

## Licença
Projeto educacional – uso acadêmico e experimental.
