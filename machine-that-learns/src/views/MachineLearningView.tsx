import { AddClassDialog } from '../components/AddClassDialog'
import { AppFooter } from '../components/AppFooter'
import { AppHeader } from '../components/AppHeader'
import { ClassTableSection } from '../components/ClassTableSection'
import { ControlRoom } from '../components/ControlRoom'
import type { MachineLearningViewModel } from '../viewmodels/useMachineLearningViewModel'

type MachineLearningViewProps = {
  viewModel: MachineLearningViewModel
}

export function MachineLearningView({ viewModel }: MachineLearningViewProps) {
  const selectedClassLabel = viewModel.selectedClass
    ? `Capturar para ${viewModel.selectedClass.name}`
    : 'Selecione uma classe'

  return (
    <main className="app-shell">
      <div className="noise-layer" aria-hidden="true" />
      <div className="glow-layer" aria-hidden="true" />

      <AppHeader
        totalSamples={viewModel.totalSamples}
        readyClasses={viewModel.readyClasses}
        classCount={viewModel.classes.length}
        isPredicting={viewModel.isPredicting}
      />

      <ControlRoom
        videoRef={viewModel.videoRef}
        isModelReady={viewModel.isModelReady}
        isCameraReady={viewModel.isCameraReady}
        status={viewModel.status}
        isPredicting={viewModel.isPredicting}
        prediction={viewModel.prediction}
        predictionConfidence={viewModel.predictionConfidence}
        trainingState={viewModel.trainingState}
        canTrain={viewModel.canTrain}
        classCount={viewModel.classes.length}
        isTraining={viewModel.isTraining}
        onTrain={viewModel.trainClassifier}
        onReset={viewModel.resetSamples}
      />

      <ClassTableSection
        classes={viewModel.classes}
        sampleCounts={viewModel.sampleCounts}
        selectedClassId={viewModel.selectedClassId}
        selectedClassLabel={selectedClassLabel}
        isCameraReady={viewModel.isCameraReady}
        selectedClassIndex={viewModel.selectedClassIndex}
        onOpenAddClassDialog={viewModel.openAddClassDialog}
        onCollectSelectedClass={() => {
          if (viewModel.selectedClassIndex >= 0) {
            viewModel.collectExample(viewModel.selectedClassIndex)
          }
        }}
        onSelectClass={viewModel.setSelectedClassId}
        onRemoveClass={viewModel.removeClass}
      />

      <AppFooter />

      <AddClassDialog
        isOpen={viewModel.isAddClassDialogOpen}
        className={viewModel.newClassName}
        onClose={viewModel.closeAddClassDialog}
        onChangeClassName={viewModel.setNewClassName}
        onSubmit={viewModel.addClass}
      />
    </main>
  )
}
