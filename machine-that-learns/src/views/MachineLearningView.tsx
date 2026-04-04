import { AddClassDialog } from '../components/AddClassDialog'
import { AppFooter } from '../components/AppFooter'
import { AppHeader } from '../components/AppHeader'
import { ClassTableSection } from '../components/ClassTableSection'
import { ControlRoom } from '../components/ControlRoom'
import { TrainingDialog } from '../components/TrainingDialog'
import type { MachineLearningViewModel } from '../viewmodels/useMachineLearningViewModel'

type MachineLearningViewProps = {
  viewModel: MachineLearningViewModel
}

export function MachineLearningView({ viewModel }: MachineLearningViewProps) {
  return (
    <main className="app-shell">
      <div className="noise-layer" aria-hidden="true" />
      <div className="glow-layer" aria-hidden="true" />

      <AppHeader />

      <ControlRoom
        videoRef={viewModel.videoRef}
        isModelReady={viewModel.isModelReady}
        isCameraReady={viewModel.isCameraReady}
        isPredicting={viewModel.isPredicting}
        prediction={viewModel.prediction}
        predictionConfidence={viewModel.predictionConfidence}
        trainingState={viewModel.trainingState}
        trainingEpochs={viewModel.trainingConfig.epochs}
        trainingProgressPercent={viewModel.trainingProgressPercent}
        totalSamples={viewModel.totalSamples}
        readyClasses={viewModel.readyClasses}
        classCount={viewModel.classes.length}
        canTrain={viewModel.canTrain}
        isTraining={viewModel.isTraining}
        canReset={viewModel.totalSamples > 0}
        canCapture={viewModel.selectedClassIndex >= 0 && viewModel.isCameraReady}
        focusedClassName={viewModel.selectedClass?.name ?? null}
        onOpenTraining={viewModel.openTrainingDialog}
        onCapture={viewModel.collectSelectedClassExample}
        onReset={viewModel.resetSamples}
      />

      <ClassTableSection
        classes={viewModel.classes}
        sampleCounts={viewModel.sampleCounts}
        selectedClassId={viewModel.selectedClassId}
        onOpenAddClassDialog={viewModel.openAddClassDialog}
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

      <TrainingDialog
        isOpen={viewModel.isTrainingDialogOpen}
        isTraining={viewModel.isTraining}
        canTrain={viewModel.canTrain}
        config={viewModel.trainingConfig}
        logs={viewModel.trainingLogs}
        progressLabel={viewModel.trainingProgressLabel}
        onClose={viewModel.closeTrainingDialog}
        onChangeConfig={viewModel.updateTrainingConfig}
        onSubmit={viewModel.trainClassifier}
        onStop={viewModel.stopTraining}
      />
    </main>
  )
}
