import { useEffect, useRef } from 'react'
import type { TrainingConfig } from '../models/machine-learning'

type TrainingDialogProps = {
  isOpen: boolean
  isTraining: boolean
  canTrain: boolean
  config: TrainingConfig
  logs: string[]
  progressLabel: string
  onClose: () => void
  onChangeConfig: (field: keyof TrainingConfig, value: number) => void
  onSubmit: () => void
  onStop: () => void
}

export function TrainingDialog({
  isOpen,
  isTraining,
  canTrain,
  config,
  logs,
  progressLabel,
  onClose,
  onChangeConfig,
  onSubmit,
  onStop,
}: TrainingDialogProps) {
  const logRef = useRef<HTMLDivElement | null>(null)

  useEffect(() => {
    if (logRef.current) {
      logRef.current.scrollTop = logRef.current.scrollHeight
    }
  }, [logs])

  if (!isOpen) {
    return null
  }

  return (
    <div className="modal-backdrop" onClick={() => !isTraining && onClose()}>
      <dialog
        className="class-dialog training-dialog"
        open
        onClick={(event) => event.stopPropagation()}
      >
        <div className="dialog-form">
          <div>
            <p className="eyebrow">Treinamento</p>
            <h3>Configurar treino do modelo</h3>
          </div>

          <div className="training-grid">
            <label>
              <span>Epocas</span>
              <input
                type="number"
                min="1"
                max="200"
                value={config.epochs}
                onChange={(event) =>
                  onChangeConfig('epochs', Number(event.target.value))
                }
                disabled={isTraining}
              />
            </label>
            <label>
              <span>Learning rate</span>
              <input
                type="number"
                min="0.00001"
                max="1"
                step="0.00001"
                value={config.learningRate}
                onChange={(event) =>
                  onChangeConfig('learningRate', Number(event.target.value))
                }
                disabled={isTraining}
              />
            </label>
            <label>
              <span>Neuronios</span>
              <input
                type="number"
                min="8"
                max="1024"
                step="8"
                value={config.hiddenUnits}
                onChange={(event) =>
                  onChangeConfig('hiddenUnits', Number(event.target.value))
                }
                disabled={isTraining}
              />
            </label>
          </div>

          <div className="training-status">
            {isTraining ? <span className="loader-spinner" aria-hidden="true" /> : null}
            <strong>{progressLabel}</strong>
          </div>

          <div ref={logRef} className="training-log" aria-live="polite">
            {logs.length > 0 ? (
              logs.map((line, index) => (
                <p key={`${index}-${line}`}>{line}</p>
              ))
            ) : (
              <p>Nenhum log ainda. Configure os parametros e inicie o treino.</p>
            )}
          </div>

          <div className="dialog-actions">
            <button
              type="button"
              className="button-secondary"
              onClick={onClose}
              disabled={isTraining}
            >
              Fechar
            </button>
            <button
              type="button"
              className="button-primary"
              onClick={onSubmit}
              disabled={!canTrain || isTraining}
            >
              {isTraining ? (
                <>
                  <span className="button-loader" aria-hidden="true" />
                  Treinando...
                </>
              ) : 'Treinar'}
            </button>
            <button
              type="button"
              className="button-secondary"
              onClick={onStop}
              disabled={!isTraining}
            >
              Parar treino
            </button>
          </div>
        </div>
      </dialog>
    </div>
  )
}
