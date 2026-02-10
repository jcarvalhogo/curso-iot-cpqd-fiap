# shared-libs

## Visão Geral

O projeto **shared-libs** concentra todas as bibliotecas reutilizáveis utilizadas nos projetos
`vehicle-device` e `gateway-arduino`. Ele foi criado para **padronizar**, **reutilizar** e **isolar**
funcionalidades comuns de IoT, rede, segurança e hardware, facilitando manutenção, testes e evolução
do ecossistema.

Este repositório **não contém aplicação final**, apenas **bibliotecas**.

---

## Objetivos Técnicos

- Reutilização de código entre múltiplos dispositivos
- Separação clara entre **aplicação** e **infraestrutura**
- Segurança de comunicação fim-a-fim
- Padronização de drivers e serviços
- Facilitar testes, simulações e futuras extensões

---

## Estrutura Geral

```
shared-libs/
├── SecureHttp/
├── WiFiManager/
├── LedStatus/
├── HttpServer/
├── GatewayClient/
├── UbidotsClient/
├── ThingSpeakClient/
├── DhtSensor/
├── FuelLevel/
└── README.md
```

Cada diretório representa uma biblioteca independente.

---

## Bibliotecas Principais

### 🔐 SecureHttp

Camada de **segurança criptográfica** para comunicação HTTP entre dispositivos IoT.

**Recursos:**
- AES-256-GCM (confidencialidade + integridade)
- HMAC-SHA256 (autenticação)
- Nonce + timestamp (anti-replay)
- Canonical request signing
- Compatível ESP32 / Arduino

Usada por:
- `GatewayClient`
- `HttpServer`

---

### 📡 WiFiManager

Gerenciador de conectividade Wi-Fi.

**Recursos:**
- Conexão automática
- Reconexão transparente
- Configuração centralizada
- Controle de potência TX
- Hostname customizado

Usada por:
- `vehicle-device`
- `gateway-arduino`

---

### 💡 LedStatus

Abstração de LED de status do sistema.

**Recursos:**
- Modos: OFF, ON, BLINK_SLOW, BLINK_FAST
- Não bloqueante
- Indicação visual de estado do sistema

Usada por:
- `gateway-arduino`

---

### 🌐 HttpServer

Servidor HTTP embarcado com segurança.

**Recursos:**
- Endpoint `/telemetry`
- Integração com SecureHttp
- Validação de payload
- Timer interno para ThingSpeak
- Callbacks de eventos

Usada por:
- `gateway-arduino`

---

### 🚗 GatewayClient

Cliente HTTP seguro para envio de telemetria.

**Recursos:**
- Comunicação segura com gateway
- Rate-limit local
- Retries e timeout
- Serial debug opcional

Usada por:
- `vehicle-device`

---

### ☁️ UbidotsClient

Cliente MQTT/HTTP para Ubidots.

**Recursos:**
- Publicação de múltiplos campos
- Tratamento de erros HTTP
- Compatível com gateway

Usada por:
- `gateway-arduino`

---

### 📊 ThingSpeakClient

Cliente HTTP para ThingSpeak.

**Recursos:**
- Rate-limit controlado
- Publicação periódica
- Integração com HttpServer

Usada por:
- `gateway-arduino`

---

### 🌡️ DhtSensor

Driver encapsulado para sensores DHT.

**Recursos:**
- DHT11 / DHT22
- Controle de intervalo mínimo
- Cache de última leitura válida

Usada por:
- `vehicle-device`

---

### ⛽ FuelLevel

Abstração de leitura analógica para nível de combustível.

**Recursos:**
- Calibração min/max
- Suavização por amostragem
- Conversão para porcentagem

Usada por:
- `vehicle-device`

---

## Arquitetura de Comunicação

```
[ Sensors ]
     ↓
vehicle-device
     ↓ SecureHttp (AES + HMAC)
gateway-arduino
     ↓
+----------+-----------+
| Ubidots  | ThingSpeak|
+----------+-----------+
```

---

## Benefícios da Arquitetura

- 🔒 Segurança de ponta a ponta
- ♻️ Código reutilizável
- 🧩 Componentes desacoplados
- 🛠️ Facilidade de manutenção
- 📈 Escalável para novos dispositivos

---

## Público-Alvo

- Projetos IoT acadêmicos e profissionais
- ESP32 / Arduino
- Sistemas embarcados conectados
- Estudos de segurança em IoT
- Provas de conceito industriais

---

## Observações Importantes

- **Nunca versionar secrets reais**
- `SecureHttpConfig.h` deve ser privado
- Bibliotecas podem ser usadas isoladamente
- Projetado para PlatformIO

---

## Licença

Uso educacional e experimental.  
Adapte conforme necessidade do projeto.

---

**shared-libs** é a fundação técnica de todo o ecossistema IoT do projeto.
