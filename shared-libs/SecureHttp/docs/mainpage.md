# SecureHttp — documentação

`SecureHttp` é uma biblioteca para **ESP32 + Arduino** que adiciona **confidencialidade**, **integridade** e **autenticação** a requisições HTTP em redes locais, **sem depender de HTTPS/TLS**.

Ela foi projetada para cenários de IoT onde:

- existe um **gateway** (ex.: ESP32 rodando `WebServer`) que recebe telemetria;
- existe um **device** (ex.: ESP32/ESP8266) que envia dados periodicamente;
- você quer impedir que terceiros na rede **leiam** ou **forjem** requisições HTTP.

## O que a biblioteca garante

- **Confidencialidade do corpo**: o payload (JSON) é cifrado com **AES-256-GCM**.
- **Integridade do corpo**: o **GCM tag** detecta modificações no ciphertext.
- **Autenticação do request**: uma assinatura **HMAC-SHA256** cobre método, rota, IDs e o corpo cifrado.
- **Anti-replay**: valida **janela de timestamp** e mantém um **cache de nonce** recente.

## O que não garante

- Não substitui TLS em cenários expostos à internet.
- Não provê gerenciamento de chaves/rotacionamento por si só.
- Não protege contra comprometimento do dispositivo (se o segredo vazar, o atacante assina requests válidos).

## Componentes

- `SecureDeviceAuth`: cria o envelope (headers + corpo cifrado) no dispositivo.
- `SecureGatewayAuth`: valida headers, HMAC, anti-replay e decripta no gateway.
- `AesGcmCodec`: helper AES-256-GCM.
- `CryptoUtils`, `HexUtils`, `NonceCache`: utilitários.

Para uma visão completa e exemplos de uso, veja o `README.md`.
