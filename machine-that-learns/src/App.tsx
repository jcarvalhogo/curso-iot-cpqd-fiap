import './App.css'
import { MachineLearningView } from './views/MachineLearningView'
import { useMachineLearningViewModel } from './viewmodels/useMachineLearningViewModel'

function App() {
  const viewModel = useMachineLearningViewModel()

  return <MachineLearningView viewModel={viewModel} />
}

export default App
