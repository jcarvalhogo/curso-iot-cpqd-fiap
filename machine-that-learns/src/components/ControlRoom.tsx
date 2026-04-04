import type { PredictionState, TrainingState } from '../models/machine-learning'

type ControlRoomProps = {
  videoRef: React.RefObject<HTMLVideoElement | null>
  isModelReady: boolean
  isCameraReady: boolean
  status: string
  isPredicting: boolean
  prediction: PredictionState | null
  predictionConfidence: number
  trainingState: TrainingState | null
  canTrain: boolean
  classCount: number
  isTraining: boolean
  onTrain: () => void
  onReset: () => void
}

export function ControlRoom({
  videoRef,
  isModelReady,
  isCameraReady,
  status,
  isPredicting,
  prediction,
  predictionConfidence,
  trainingState,
  canTrain,
  classCount,
  isTraining,
  onTrain,
  onReset,
}: ControlRoomProps) {
  return (
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
            <strong>{trainingState ? trainingState.loss.toFixed(4) : '--'}</strong>
          </div>
          <div className="telemetry-row">
            <span>Ultima epoca</span>
            <strong>{trainingState ? trainingState.epoch : '--'}</strong>
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
              onClick={onTrain}
              disabled={!canTrain || isTraining}
            >
              {isTraining ? 'Treinando...' : 'Treinar modelo'}
            </button>
            <button type="button" className="button-secondary" onClick={onReset}>
              Limpar amostras
            </button>
          </div>
          <div className="telemetry-row">
            <span>Pronto para treino</span>
            <strong>{canTrain ? 'Sim' : 'Nao'}</strong>
          </div>
          <div className="telemetry-row">
            <span>Total de classes</span>
            <strong>{classCount}</strong>
          </div>
        </div>
      </aside>
    </section>
  )
}
