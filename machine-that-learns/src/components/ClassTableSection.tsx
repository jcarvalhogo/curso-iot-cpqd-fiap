import { TARGET_SAMPLES_PER_CLASS, type ClassItem } from '../models/machine-learning'

type ClassTableSectionProps = {
  classes: ClassItem[]
  sampleCounts: number[]
  selectedClassId: number
  selectedClassLabel: string
  isCameraReady: boolean
  selectedClassIndex: number
  onOpenAddClassDialog: () => void
  onCollectSelectedClass: () => void
  onSelectClass: (classId: number) => void
  onRemoveClass: (index: number) => void
}

export function ClassTableSection({
  classes,
  sampleCounts,
  selectedClassId,
  selectedClassLabel,
  isCameraReady,
  selectedClassIndex,
  onOpenAddClassDialog,
  onCollectSelectedClass,
  onSelectClass,
  onRemoveClass,
}: ClassTableSectionProps) {
  return (
    <section className="class-lab">
      <div className="section-title">
        <div className="section-heading">
          <p className="eyebrow">Dataset</p>
          <div className="section-heading-row">
            <h2>Lista de classes</h2>
            <button
              type="button"
              className="help-trigger"
              aria-label="Instrucoes da tabela"
            >
              i
              <span className="help-tooltip" role="tooltip">
                Crie quantas classes precisar para teste. Alterar a lista limpa
                as amostras atuais para evitar inconsistencias no treino.
              </span>
            </button>
          </div>
        </div>
      </div>

      <div className="class-toolbar">
        <button
          type="button"
          className="button-primary"
          onClick={onOpenAddClassDialog}
        >
          Adicionar classe
        </button>
        <button
          type="button"
          className="capture-button"
          onClick={onCollectSelectedClass}
          disabled={!isCameraReady || selectedClassIndex < 0}
        >
          {selectedClassLabel}
        </button>
      </div>

      <div className="table-shell">
        <table className="class-table">
          <thead>
            <tr>
              <th aria-label="Selecionar" />
              <th>Classe</th>
              <th>Amostras</th>
              <th>Progresso</th>
              <th>Status</th>
              <th>Acoes</th>
            </tr>
          </thead>
          <tbody>
            {classes.map((classItem, index) => {
              const sampleCount = sampleCounts[index]
              const progress = Math.min(
                100,
                (sampleCount / TARGET_SAMPLES_PER_CLASS) * 100,
              )

              return (
                <tr
                  key={classItem.id}
                  className={selectedClassId === classItem.id ? 'is-selected' : ''}
                  onClick={() => onSelectClass(classItem.id)}
                >
                  <td>
                    <span
                      className={`row-selector ${selectedClassId === classItem.id ? 'is-active' : ''}`}
                      aria-hidden="true"
                    />
                  </td>
                  <td>
                    <span className="class-chip">
                      {classItem.name || `Classe ${index + 1}`}
                    </span>
                  </td>
                  <td>{sampleCount}</td>
                  <td>
                    <div className="progress-track table-progress" aria-hidden="true">
                      <div
                        className="progress-fill"
                        style={{ width: `${progress}%` }}
                      />
                    </div>
                  </td>
                  <td>
                    <span className="sample-hint sample-hint-inline">
                      {sampleCount >= TARGET_SAMPLES_PER_CLASS
                        ? `Minimo recomendado: ${TARGET_SAMPLES_PER_CLASS}`
                        : `Faltam ${Math.max(0, TARGET_SAMPLES_PER_CLASS - sampleCount)} de ${TARGET_SAMPLES_PER_CLASS}`}
                    </span>
                  </td>
                  <td>
                    <button
                      type="button"
                      className="button-secondary button-compact"
                      onClick={(event) => {
                        event.stopPropagation()
                        onRemoveClass(index)
                      }}
                    >
                      Remover
                    </button>
                  </td>
                </tr>
              )
            })}
          </tbody>
        </table>
      </div>
    </section>
  )
}
