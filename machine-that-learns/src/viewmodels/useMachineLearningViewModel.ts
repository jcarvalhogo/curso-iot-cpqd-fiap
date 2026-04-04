import * as mobilenet from '@tensorflow-models/mobilenet'
import * as tf from '@tensorflow/tfjs'
import { useEffect, useMemo, useRef, useState, type FormEvent } from 'react'
import {
  DEFAULT_TRAINING_CONFIG,
  INITIAL_CLASSES,
  MIN_SAMPLES_PER_CLASS,
  type PredictionState,
  type TrainingConfig,
  type TrainingState,
} from '../models/machine-learning'

export function useMachineLearningViewModel() {
  const videoRef = useRef<HTMLVideoElement | null>(null)
  const streamRef = useRef<MediaStream | null>(null)
  const mobilenetRef = useRef<mobilenet.MobileNet | null>(null)
  const classifierRef = useRef<tf.Sequential | null>(null)
  const animationFrameRef = useRef<number | null>(null)
  const classNamesRef = useRef<string[]>([])
  const trainingExamplesRef = useRef<tf.Tensor2D[]>([])
  const trainingLabelsRef = useRef<number[]>([])
  const nextClassIdRef = useRef(1)
  const stopTrainingRequestedRef = useRef(false)

  const [classes, setClasses] = useState(INITIAL_CLASSES)
  const [sampleCounts, setSampleCounts] = useState<number[]>([])
  const [isModelReady, setIsModelReady] = useState(false)
  const [isCameraReady, setIsCameraReady] = useState(false)
  const [isTraining, setIsTraining] = useState(false)
  const [isPredicting, setIsPredicting] = useState(false)
  const [status, setStatus] = useState(
    'Carregando MobileNet e preparando a webcam...',
  )
  const [prediction, setPrediction] = useState<PredictionState | null>(null)
  const [trainingState, setTrainingState] = useState<TrainingState | null>(null)
  const [selectedClassId, setSelectedClassId] = useState<number | null>(null)
  const [isAddClassDialogOpen, setIsAddClassDialogOpen] = useState(false)
  const [newClassName, setNewClassName] = useState('')
  const [isTrainingDialogOpen, setIsTrainingDialogOpen] = useState(false)
  const [trainingConfig, setTrainingConfig] = useState<TrainingConfig>(
    DEFAULT_TRAINING_CONFIG,
  )
  const [trainingLogs, setTrainingLogs] = useState<string[]>([])

  const canTrain = useMemo(
    () =>
      classes.length >= 2 &&
      sampleCounts.length === classes.length &&
      sampleCounts.every((count) => count >= MIN_SAMPLES_PER_CLASS),
    [classes.length, sampleCounts],
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
  const selectedClassIndex = useMemo(
    () => classes.findIndex((classItem) => classItem.id === selectedClassId),
    [classes, selectedClassId],
  )
  const selectedClass =
    selectedClassIndex >= 0 ? classes[selectedClassIndex] : null
  const trainingProgressLabel = trainingState
    ? `Epoca ${trainingState.epoch}/${trainingConfig.epochs} · loss ${trainingState.loss.toFixed(4)} · acc ${(((trainingState.accuracy ?? 0) * 100)).toFixed(1)}%`
    : isTraining
      ? 'Preparando dados e inicializando treino...'
      : 'Aguardando inicio do treino'
  const trainingProgressPercent =
    trainingState && trainingConfig.epochs > 0
      ? Math.min(100, Math.round((trainingState.epoch / trainingConfig.epochs) * 100))
      : 0

  classNamesRef.current = classes.map((item) => item.name)

  function stopPredictionLoop() {
    if (animationFrameRef.current !== null) {
      window.cancelAnimationFrame(animationFrameRef.current)
      animationFrameRef.current = null
    }
    setIsPredicting(false)
  }

  function clearTrainingData() {
    stopPredictionLoop()
    classifierRef.current?.dispose()
    classifierRef.current = null
    trainingExamplesRef.current.forEach((tensor) => tensor.dispose())
    trainingExamplesRef.current = []
    trainingLabelsRef.current = []
    setPrediction(null)
    setTrainingState(null)
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
        setStatus('Webcam pronta. Adicione classes para comecar os testes.')
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

  function openAddClassDialog() {
    setNewClassName(`Classe ${classes.length + 1}`)
    setIsAddClassDialogOpen(true)
  }

  function closeAddClassDialog() {
    setIsAddClassDialogOpen(false)
    setNewClassName('')
  }

  function addClass(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    clearTrainingData()

    const nextClassNumber = nextClassIdRef.current
    nextClassIdRef.current += 1
    const className = newClassName.trim() || `Classe ${classes.length + 1}`

    setClasses((currentClasses) => [
      ...currentClasses,
      { id: nextClassNumber, name: className },
    ])
    setSampleCounts((currentCounts) => [...currentCounts, 0])
    setSelectedClassId(nextClassNumber)
    closeAddClassDialog()
    setStatus('Nova classe adicionada. Selecione a linha e capture exemplos.')
  }

  function removeClass(index: number) {
    if (classes.length === 1) {
      setStatus('Mantenha pelo menos uma classe na lista.')
      return
    }

    clearTrainingData()
    setClasses((currentClasses) =>
      currentClasses.filter((_, currentIndex) => currentIndex !== index),
    )
    setSampleCounts((currentCounts) =>
      currentCounts.filter((_, currentIndex) => currentIndex !== index),
    )

    const fallbackClass = classes.find((_, currentIndex) => currentIndex !== index)
    setSelectedClassId(fallbackClass ? fallbackClass.id : null)
    setStatus(
      'Classe removida. As amostras foram limpas para manter o teste consistente.',
    )
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

    setStatus(`Exemplo salvo para ${classes[classIndex].name}.`)
  }

  function openTrainingDialog() {
    setIsTrainingDialogOpen(true)
    setTrainingLogs((currentLogs) =>
      currentLogs.length > 0
        ? currentLogs
        : ['Configure os parametros para iniciar o treino do modelo.'],
    )
  }

  function closeTrainingDialog() {
    if (!isTraining) {
      setIsTrainingDialogOpen(false)
    }
  }

  function updateTrainingConfig(field: keyof TrainingConfig, value: number) {
    if (!Number.isFinite(value) || value <= 0) {
      return
    }

    setTrainingConfig((currentConfig) => ({
      ...currentConfig,
      [field]: value,
    }))
  }

  async function trainClassifier() {
    if (!canTrain || trainingExamplesRef.current.length === 0) {
      setStatus(
        'Use pelo menos duas classes e adicione um exemplo em cada uma antes de treinar.',
      )
      return
    }

    stopPredictionLoop()
    classifierRef.current?.dispose()
    stopTrainingRequestedRef.current = false
    setIsTraining(true)
    setPrediction(null)
    setTrainingState(null)
    setTrainingLogs([
      `Treino iniciado com ${classes.length} classes e ${trainingExamplesRef.current.length} amostras.`,
      `Parametros: ${trainingConfig.epochs} epocas, learning rate ${trainingConfig.learningRate}, ${trainingConfig.hiddenUnits} neuronios.`,
      'Preparando tensores e configurando o modelo...',
    ])
    setStatus('Treinando classificador no navegador...')

    // Give React a frame to paint the loading state before heavy tensor work starts.
    await tf.nextFrame()
    await new Promise<void>((resolve) => {
      window.setTimeout(() => resolve(), 0)
    })

    const inputs = tf.concat(trainingExamplesRef.current) as tf.Tensor2D
    const labels = tf.tensor1d(trainingLabelsRef.current, 'int32')
    const classCount = classes.length
    const oneHotLabels = tf.oneHot(labels, classCount)

    const classifier = tf.sequential({
      layers: [
        tf.layers.dense({
          inputShape: [inputs.shape[1]],
          units: trainingConfig.hiddenUnits,
          activation: 'relu',
        }),
        tf.layers.dense({
          units: classCount,
          activation: 'softmax',
        }),
      ],
    })

    classifier.compile({
      optimizer: tf.train.adam(trainingConfig.learningRate),
      loss: 'categoricalCrossentropy',
      metrics: ['accuracy'],
    })

    try {
      await classifier.fit(inputs, oneHotLabels, {
        epochs: trainingConfig.epochs,
        shuffle: true,
        callbacks: {
          onEpochEnd: async (epoch, logs) => {
            setTrainingState({
              epoch: epoch + 1,
              loss: logs?.loss ?? 0,
              accuracy:
                typeof logs?.acc === 'number'
                  ? logs.acc
                  : typeof logs?.accuracy === 'number'
                    ? logs.accuracy
                    : null,
            })
            setTrainingLogs((currentLogs) => [
              ...currentLogs,
              `Epoca ${epoch + 1}/${trainingConfig.epochs} finalizada com loss ${(logs?.loss ?? 0).toFixed(4)} e acc ${(((typeof logs?.acc === 'number' ? logs.acc : typeof logs?.accuracy === 'number' ? logs.accuracy : 0) * 100)).toFixed(1)}%.`,
            ])
            if (stopTrainingRequestedRef.current) {
              classifier.stopTraining = true
            }
            await tf.nextFrame()
          },
        },
      })

      classifierRef.current = classifier

      if (stopTrainingRequestedRef.current) {
        setStatus('Treino interrompido pelo usuario.')
        setTrainingLogs((currentLogs) => [
          ...currentLogs,
          'Treino interrompido pelo usuario.',
        ])
      } else {
        setStatus('Treino concluido. Executando predicao em tempo real.')
        setTrainingLogs((currentLogs) => [
          ...currentLogs,
          'Treino concluido. Iniciando predicao em tempo real.',
        ])
        predictFrame()
      }
    } catch (error) {
      console.error(error)
      classifier.dispose()
      setStatus('O treino falhou. Revise as amostras coletadas e tente novamente.')
      setTrainingLogs((currentLogs) => [
        ...currentLogs,
        'Treino interrompido por erro. Revise as configuracoes e as amostras.',
      ])
    } finally {
      stopTrainingRequestedRef.current = false
      inputs.dispose()
      labels.dispose()
      oneHotLabels.dispose()
      setIsTraining(false)
    }
  }

  function stopTraining() {
    if (!isTraining) {
      return
    }

    stopTrainingRequestedRef.current = true
    setTrainingLogs((currentLogs) => [
      ...currentLogs,
      'Solicitacao para parar o treino enviada. Aguardando fim da epoca atual.',
    ])
    setStatus('Parando treino...')
  }

  function resetSamples() {
    clearTrainingData()
    setSampleCounts(Array(classes.length).fill(0))
    setStatus('Amostras limpas. Colete novos exemplos para treinar novamente.')
  }

  function collectSelectedClassExample() {
    if (selectedClassIndex < 0) {
      setStatus('Selecione uma classe na tabela antes de capturar um exemplo.')
      return
    }

    collectExample(selectedClassIndex)
  }

  return {
    videoRef,
    classes,
    sampleCounts,
    isModelReady,
    isCameraReady,
    isTraining,
    isPredicting,
    status,
    prediction,
    trainingState,
    selectedClassId,
    isAddClassDialogOpen,
    isTrainingDialogOpen,
    newClassName,
    trainingConfig,
    trainingLogs,
    canTrain,
    totalSamples,
    readyClasses,
    predictionConfidence,
    selectedClassIndex,
    selectedClass,
    trainingProgressLabel,
    trainingProgressPercent,
    setSelectedClassId,
    setNewClassName,
    openAddClassDialog,
    closeAddClassDialog,
    openTrainingDialog,
    closeTrainingDialog,
    updateTrainingConfig,
    addClass,
    removeClass,
    collectExample,
    collectSelectedClassExample,
    trainClassifier,
    stopTraining,
    resetSamples,
  }
}

export type MachineLearningViewModel = ReturnType<
  typeof useMachineLearningViewModel
>
