export type PredictionState = {
  className: string
  confidence: number
}

export type TrainingState = {
  epoch: number
  loss: number
}

export type ClassItem = {
  id: number
  name: string
}

export type TrainingConfig = {
  epochs: number
  learningRate: number
  hiddenUnits: number
}

export const MIN_SAMPLES_PER_CLASS = 1
export const TARGET_SAMPLES_PER_CLASS = 12
export const GUIDE_LINES = [
  'Evite usar exatamente o mesmo fundo para todas as classes.',
  'Inclua mudancas de distancia, luz e orientacao.',
  'Refaca o treino se a predicao oscilar demais.',
] as const

export const INITIAL_CLASSES: ClassItem[] = []
export const DEFAULT_TRAINING_CONFIG: TrainingConfig = {
  epochs: 20,
  learningRate: 0.0001,
  hiddenUnits: 128,
}
