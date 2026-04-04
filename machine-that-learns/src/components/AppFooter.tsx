import { GUIDE_LINES } from '../models/machine-learning'

export function AppFooter() {
  return (
    <footer className="app-footer">
      <p className="eyebrow">React + TensorFlow.js</p>
      <div className="footer-guide">
        <button type="button" className="eyebrow footer-guide-trigger">
          Guia rapido
          <span className="help-tooltip footer-tooltip" role="tooltip">
            {GUIDE_LINES.map((line) => (
              <span key={line} className="footer-tooltip-line">
                {line}
              </span>
            ))}
            <a
              className="guide-link"
              href="https://codelabs.developers.google.com/tensorflowjs-transfer-learning-teachable-machine?hl=pt-br#0"
              target="_blank"
              rel="noreferrer"
            >
              Abrir o tutorial base
            </a>
          </span>
        </button>
      </div>
    </footer>
  )
}
