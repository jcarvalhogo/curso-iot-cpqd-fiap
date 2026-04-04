# Machine That Learns

Projeto React inspirado no codelab oficial do TensorFlow.js sobre aprendizado por transferencia com webcam:

https://codelabs.developers.google.com/tensorflowjs-transfer-learning-teachable-machine?hl=pt-br#0

## O que este app faz

- Carrega o MobileNet no navegador com TensorFlow.js.
- Abre a webcam do usuario.
- Permite capturar exemplos para 3 classes customizadas.
- Treina um classificador pequeno diretamente no browser.
- Executa predicao em tempo real usando a webcam.

## Como rodar

```bash
npm install
npm run dev
```

## Fluxo de uso

1. Permita acesso a webcam.
2. Renomeie as classes se quiser.
3. Capture alguns exemplos para cada classe.
4. Clique em `Treinar modelo`.
5. Teste a predicao em tempo real.

## Observacoes

- O treinamento acontece localmente no navegador.
- Quanto mais variados forem os exemplos, melhor tende a ser a classificacao.
- Ainda nao ha persistencia de modelo treinado nem exportacao/importacao.
