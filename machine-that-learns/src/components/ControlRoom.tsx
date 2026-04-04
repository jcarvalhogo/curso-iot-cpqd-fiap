import type { PredictionState, TrainingState } from '../models/machine-learning'

type ControlRoomProps = {
  videoRef: React.RefObject<HTMLVideoElement | null>
  isModelReady: boolean
  isCameraReady: boolean
  isPredicting: boolean
  prediction: PredictionState | null
  predictionConfidence: number
  trainingState: TrainingState | null
  totalSamples: number
  readyClasses: number
  classCount: number
  canTrain: boolean
  isTraining: boolean
  canReset: boolean
  onOpenTraining: () => void
  onReset: () => void
}

export function ControlRoom({
  videoRef,
  isModelReady,
  isCameraReady,
  isPredicting,
  prediction,
  predictionConfidence,
  trainingState,
  totalSamples,
  readyClasses,
  classCount,
  canTrain,
  isTraining,
  canReset,
  onOpenTraining,
  onReset,
}: ControlRoomProps) {
  return (
    <section className="control-room">
      <div className="telemetry-card dataset-side-card">
        <div>
          <p className="eyebrow">Dataset</p>
          <h2 className="telemetry-title">Resumo do treino</h2>
        </div>
        <div className="telemetry-row">
          <span>Total de classes</span>
          <strong>{classCount}</strong>
        </div>
        <div className="telemetry-row">
          <span>Classes prontas</span>
          <strong>{readyClasses}</strong>
        </div>
        <div className="telemetry-row">
          <span>Amostras totais</span>
          <strong>{totalSamples}</strong>
        </div>
        <div className="action-row action-row-stacked">
          <button
            type="button"
            className="button-primary"
            onClick={onOpenTraining}
            disabled={!canTrain}
          >
            {isTraining ? 'Treinando...' : 'Treinar modelo'}
          </button>
          <button
            type="button"
            className="button-secondary"
            onClick={onReset}
            disabled={!canReset || isTraining}
          >
            Limpar amostras
          </button>
        </div>
      </div>

      <div className="telemetry-card monitor-card">
        <div className="monitor-head">
          <div className="stage-header">
            <div>
              <p className="eyebrow">Monitor</p>
            </div>
          </div>

          <div className="stage-status-icon" aria-label="Status da webcam">
            <span className={`dot ${isModelReady && isCameraReady ? 'ready' : ''}`} />
          </div>
        </div>

        <div className="stage-body">
          <div className="stage-shell">
            <video ref={videoRef} muted playsInline className="webcam" />
          </div>
        </div>
      </div>

      <div className="telemetry-card">
        <div className="telemetry-head">
          <div>
            <p className="eyebrow">Predicao</p>
            <h2 className="telemetry-title">
              {prediction ? prediction.className : 'Sem resultado'}
            </h2>
          </div>
          <strong className="telemetry-value">
            {prediction ? `${predictionConfidence}%` : '--'}
          </strong>
        </div>
        <div className="prediction-bar" aria-hidden="true">
          <div className="prediction-fill" style={{ width: `${predictionConfidence}%` }} />
        </div>
        <div className="telemetry-row">
          <span>Estado</span>
          <strong>{isPredicting ? 'Executando' : 'Parado'}</strong>
        </div>
        <div className="telemetry-row">
          <span>Loss atual</span>
          <strong>{trainingState ? trainingState.loss.toFixed(4) : '--'}</strong>
        </div>
        <div className="telemetry-row">
          <span>Ultima epoca</span>
          <strong>{trainingState ? trainingState.epoch : '--'}</strong>
        </div>
      </div>
    </section>
  )
}
