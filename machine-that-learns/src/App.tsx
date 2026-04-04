import * as mobilenet from '@tensorflow-models/mobilenet'
import * as tf from '@tensorflow/tfjs'
import {
  useEffect,
  useMemo,
  useRef,
  useState,
  type ChangeEvent,
} from 'react'
import './App.css'

type PredictionState = {
  className: string
  confidence: number
}

type TrainingState = {
  epoch: number
  loss: number
}

const CLASS_COUNT = 3
const DEFAULT_CLASS_NAMES = ['Classe 1', 'Classe 2', 'Classe 3']
const MIN_SAMPLES_PER_CLASS = 1
const TARGET_SAMPLES_PER_CLASS = 12

function App() {
  const videoRef = useRef<HTMLVideoElement | null>(null)
  const streamRef = useRef<MediaStream | null>(null)
  const mobilenetRef = useRef<mobilenet.MobileNet | null>(null)
  const classifierRef = useRef<tf.Sequential | null>(null)
  const animationFrameRef = useRef<number | null>(null)
  const classNamesRef = useRef(DEFAULT_CLASS_NAMES)
  const trainingExamplesRef = useRef<tf.Tensor2D[]>([])
  const trainingLabelsRef = useRef<number[]>([])

  const [classNames, setClassNames] = useState(DEFAULT_CLASS_NAMES)
  const [sampleCounts, setSampleCounts] = useState<number[]>(
    Array(CLASS_COUNT).fill(0),
  )
  const [isModelReady, setIsModelReady] = useState(false)
  const [isCameraReady, setIsCameraReady] = useState(false)
  const [isTraining, setIsTraining] = useState(false)
  const [isPredicting, setIsPredicting] = useState(false)
  const [status, setStatus] = useState(
    'Carregando MobileNet e preparando a webcam...',
  )
  const [prediction, setPrediction] = useState<PredictionState | null>(null)
  const [trainingState, setTrainingState] = useState<TrainingState | null>(null)

  const canTrain = useMemo(
    () => sampleCounts.every((count) => count >= MIN_SAMPLES_PER_CLASS),
    [sampleCounts],
  )
  const totalSamples = useMemo(
    () => sampleCounts.reduce((sum, count) => sum + count, 0),
    [sampleCounts],
  )
  const readyClasses = useMemo(
    () => sampleCounts.filter((count) => count >= MIN_SAMPLES_PER_CLASS).length,
    [sampleCounts],
  )
  const predictionConfidence = prediction
    ? Math.round(prediction.confidence * 100)
    : 0

  classNamesRef.current = classNames

  function stopPredictionLoop() {
    if (animationFrameRef.current !== null) {
      window.cancelAnimationFrame(animationFrameRef.current)
      animationFrameRef.current = null
    }
    setIsPredicting(false)
  }

  function predictFrame() {
    const video = videoRef.current
    const featureExtractor = mobilenetRef.current
    const classifier = classifierRef.current

    if (!video || !featureExtractor || !classifier || video.readyState < 2) {
      animationFrameRef.current = window.requestAnimationFrame(predictFrame)
      return
    }

    const result = tf.tidy(() => {
      const activation = featureExtractor.infer(video, true) as tf.Tensor2D
      const predictionTensor = classifier.predict(activation) as tf.Tensor2D
      const scores = predictionTensor.dataSync()

      let topIndex = 0
      for (let index = 1; index < scores.length; index += 1) {
        if (scores[index] > scores[topIndex]) {
          topIndex = index
        }
      }

      return {
        classIndex: topIndex,
        confidence: scores[topIndex],
      }
    })

    setPrediction({
      className: classNamesRef.current[result.classIndex],
      confidence: result.confidence,
    })
    setIsPredicting(true)

    animationFrameRef.current = window.requestAnimationFrame(predictFrame)
  }

  useEffect(() => {
    let isMounted = true

    async function setup() {
      try {
        await tf.ready()
        const loadedModel = await mobilenet.load({ version: 2, alpha: 1 })
        if (!isMounted) {
          return
        }
        mobilenetRef.current = loadedModel
        setIsModelReady(true)
        setStatus('Modelo carregado. Solicitando acesso a webcam...')

        const stream = await navigator.mediaDevices.getUserMedia({
          video: {
            width: { ideal: 640 },
            height: { ideal: 480 },
            facingMode: 'user',
          },
          audio: false,
        })

        if (!isMounted) {
          stream.getTracks().forEach((track) => track.stop())
          return
        }

        streamRef.current = stream
        const video = videoRef.current
        if (!video) {
          setStatus('Elemento de video nao encontrado.')
          return
        }

        video.srcObject = stream
        await video.play()
        setIsCameraReady(true)
        setStatus('Webcam pronta. Colete exemplos para cada classe.')
      } catch (error) {
        console.error(error)
        setStatus(
          'Nao foi possivel iniciar o app. Verifique permissao da webcam e dependencias do TensorFlow.js.',
        )
      }
    }

    void setup()

    return () => {
      isMounted = false
      stopPredictionLoop()
      classifierRef.current?.dispose()
      trainingExamplesRef.current.forEach((tensor) => tensor.dispose())
      trainingExamplesRef.current = []
      trainingLabelsRef.current = []
      streamRef.current?.getTracks().forEach((track) => track.stop())
    }
  }, [])

  function handleClassNameChange(index: number, event: ChangeEvent<HTMLInputElement>) {
    const nextClassNames = [...classNames]
    nextClassNames[index] = event.target.value || DEFAULT_CLASS_NAMES[index]
    setClassNames(nextClassNames)
  }

  function collectExample(classIndex: number) {
    const video = videoRef.current
    const featureExtractor = mobilenetRef.current

    if (!video || !featureExtractor || video.readyState < 2) {
      setStatus('A webcam ainda nao esta pronta para capturar exemplos.')
      return
    }

    const activation = tf.tidy(
      () => featureExtractor.infer(video, true) as tf.Tensor2D,
    )

    trainingExamplesRef.current.push(activation.clone())
    trainingLabelsRef.current.push(classIndex)
    activation.dispose()

    setSampleCounts((currentCounts) => {
      const nextCounts = [...currentCounts]
      nextCounts[classIndex] += 1
      return nextCounts
    })

    setStatus(`Exemplo salvo para ${classNames[classIndex]}.`)
  }

  async function trainClassifier() {
    if (!canTrain || trainingExamplesRef.current.length === 0) {
      setStatus('Adicione pelo menos um exemplo para cada classe antes de treinar.')
      return
    }

    stopPredictionLoop()
    classifierRef.current?.dispose()
    setIsTraining(true)
    setPrediction(null)
    setTrainingState(null)
    setStatus('Treinando classificador no navegador...')

    const inputs = tf.concat(trainingExamplesRef.current) as tf.Tensor2D
    const labels = tf.tensor1d(trainingLabelsRef.current, 'int32')
    const oneHotLabels = tf.oneHot(labels, CLASS_COUNT)

    const classifier = tf.sequential({
      layers: [
        tf.layers.dense({
          inputShape: [inputs.shape[1]],
          units: 128,
          activation: 'relu',
        }),
        tf.layers.dense({
          units: CLASS_COUNT,
          activation: 'softmax',
        }),
      ],
    })

    classifier.compile({
      optimizer: tf.train.adam(0.0001),
      loss: 'categoricalCrossentropy',
      metrics: ['accuracy'],
    })

    try {
      await classifier.fit(inputs, oneHotLabels, {
        epochs: 20,
        shuffle: true,
        callbacks: {
          onEpochEnd: (epoch, logs) => {
            setTrainingState({
              epoch: epoch + 1,
              loss: logs?.loss ?? 0,
            })
          },
        },
      })

      classifierRef.current = classifier
      setStatus('Treino concluido. Executando predicao em tempo real.')
      predictFrame()
    } catch (error) {
      console.error(error)
      classifier.dispose()
      setStatus('O treino falhou. Revise as amostras coletadas e tente novamente.')
    } finally {
      inputs.dispose()
      labels.dispose()
      oneHotLabels.dispose()
      setIsTraining(false)
    }
  }

  function resetSamples() {
    stopPredictionLoop()
    classifierRef.current?.dispose()
    classifierRef.current = null
    trainingExamplesRef.current.forEach((tensor) => tensor.dispose())
    trainingExamplesRef.current = []
    trainingLabelsRef.current = []
    setSampleCounts(Array(CLASS_COUNT).fill(0))
    setPrediction(null)
    setTrainingState(null)
    setStatus('Amostras limpas. Colete novos exemplos para treinar novamente.')
  }

  return (
    <main className="app-shell">
      <div className="noise-layer" aria-hidden="true" />
      <div className="glow-layer" aria-hidden="true" />

      <section className="topbar">
        <div className="brand-block">
          <p className="eyebrow">React + TensorFlow.js</p>
          <div className="brand-line">
            <h1>Machine That Learns</h1>
          </div>
          <p className="lede">
            Uma versao React do fluxo do Teachable Machine: capture exemplos pela
            webcam, treine no navegador e classifique em tempo real.
          </p>
        </div>
        <div className="badge-row">
          <article className="metric-pill">
            <span>Amostras</span>
            <strong>{totalSamples}</strong>
          </article>
          <article className="metric-pill">
            <span>Classes prontas</span>
            <strong>{readyClasses}/{CLASS_COUNT}</strong>
          </article>
          <article className="metric-pill">
            <span>Modo</span>
            <strong>{isPredicting ? 'Ao vivo' : 'Coleta'}</strong>
          </article>
        </div>
      </section>

      <section className="control-room">
        <div className="video-panel">
          <div className="stage-header">
            <div>
              <p className="eyebrow">Monitor</p>
              <h2>Visao da webcam</h2>
            </div>
            <div className="stage-status">
              <span
                className={`dot ${isModelReady && isCameraReady ? 'ready' : ''}`}
              />
              <span>{status}</span>
            </div>
          </div>
          <div className="stage-shell">
            <video ref={videoRef} muted playsInline className="webcam" />
            <div className="hud-corner">
              {isPredicting ? 'ANALISANDO' : 'AGUARDANDO'}
            </div>
          </div>
          <div className="hint-strip">
            <article className="hint-card">
              <span>01</span>
              <p>Nomeie classes com intencao clara.</p>
            </article>
            <article className="hint-card">
              <span>02</span>
              <p>Capture variacoes reais do mesmo objeto.</p>
            </article>
            <article className="hint-card">
              <span>03</span>
              <p>Treine e compare a confianca ao vivo.</p>
            </article>
          </div>
        </div>

        <aside className="telemetry-panel">
          <div className="telemetry-card">
            <div className="telemetry-head">
              <div>
                <p className="eyebrow">Predicao</p>
                <h2>{prediction ? prediction.className : 'Sem resultado'}</h2>
              </div>
              <strong className="telemetry-value">
                {prediction ? `${predictionConfidence}%` : '--'}
              </strong>
            </div>
            <div className="prediction-bar" aria-hidden="true">
              <div
                className="prediction-fill"
                style={{ width: `${predictionConfidence}%` }}
              />
            </div>
            <div className="telemetry-row">
              <span>Estado</span>
              <strong>{isPredicting ? 'Executando' : 'Parado'}</strong>
            </div>
            <div className="telemetry-row">
              <span>Loss atual</span>
              <strong>
                {trainingState ? trainingState.loss.toFixed(4) : '--'}
              </strong>
            </div>
            <div className="telemetry-row">
              <span>Ultima epoca</span>
              <strong>
                {trainingState ? trainingState.epoch : '--'}
              </strong>
            </div>
          </div>

          <div className="actions-card">
            <div>
              <p className="eyebrow">Acoes</p>
              <h3>Treinamento local</h3>
            </div>
            <div className="action-row">
              <button
                type="button"
                className="button-primary"
                onClick={trainClassifier}
                disabled={!canTrain || isTraining}
              >
                {isTraining ? 'Treinando...' : 'Treinar modelo'}
              </button>
              <button
                type="button"
                className="button-secondary"
                onClick={resetSamples}
              >
                Limpar amostras
              </button>
            </div>
            <div className="telemetry-row">
              <span>Pronto para treino</span>
              <strong>{canTrain ? 'Sim' : 'Nao'}</strong>
            </div>
          </div>

          <div className="guide-card">
            <p className="eyebrow">Guia rapido</p>
            <ul className="guide-list">
              <li>Evite usar exatamente o mesmo fundo para todas as classes.</li>
              <li>Inclua mudancas de distancia, luz e orientacao.</li>
              <li>Refaca o treino se a predicao oscilar demais.</li>
            </ul>
            <a
              className="guide-link"
              href="https://codelabs.developers.google.com/tensorflowjs-transfer-learning-teachable-machine?hl=pt-br#0"
              target="_blank"
              rel="noreferrer"
            >
              Abrir o tutorial base
            </a>
          </div>
        </aside>
      </section>

      <section className="class-lab">
        <div className="section-title">
          <div>
            <p className="eyebrow">Dataset</p>
            <h2>Classes e amostras</h2>
          </div>
          <p>
            Monte um conjunto equilibrado. A barra indica progresso rumo a uma
            meta visual de {TARGET_SAMPLES_PER_CLASS} amostras por classe.
          </p>
        </div>

        <div className="class-grid">
          {classNames.map((className, index) => {
            const sampleCount = sampleCounts[index]
            const progress = Math.min(
              100,
              (sampleCount / TARGET_SAMPLES_PER_CLASS) * 100,
            )

            return (
              <article key={index} className="class-card">
                <div className="class-card-header">
                  <span className="class-chip">Classe {index + 1}</span>
                  <span className="sample-total">{sampleCount} amostras</span>
                </div>
                <label htmlFor={`class-${index}`}>Nome da categoria</label>
                <input
                  id={`class-${index}`}
                  type="text"
                  value={className}
                  onChange={(event) => handleClassNameChange(index, event)}
                />
                <div className="progress-track" aria-hidden="true">
                  <div
                    className="progress-fill"
                    style={{ width: `${progress}%` }}
                  />
                </div>
                <p className="sample-hint">
                  {sampleCount >= TARGET_SAMPLES_PER_CLASS
                    ? 'Cobertura suficiente para uma classe mais estavel.'
                    : `Ainda faltam ${Math.max(0, TARGET_SAMPLES_PER_CLASS - sampleCount)} amostras para a meta visual.`}
                </p>
                <button
                  type="button"
                  className="capture-button"
                  onClick={() => collectExample(index)}
                  disabled={!isCameraReady}
                >
                  Capturar exemplo
                </button>
              </article>
            )
          })}
        </div>
      </section>
    </main>
  )
}

export default App
