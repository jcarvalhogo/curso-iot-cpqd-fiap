type AppHeaderProps = {
  totalSamples: number
  readyClasses: number
  classCount: number
  isPredicting: boolean
}

export function AppHeader({
  totalSamples,
  readyClasses,
  classCount,
  isPredicting,
}: AppHeaderProps) {
  return (
    <section className="topbar">
      <div className="brand-header">
        <div className="logo-slot logo-slot-left">
          <img
            src="https://d3dicilwvhavgk.cloudfront.net/cpqd-vlec-public/productcustom/logologin/638048833004891759/default.png"
            alt="CPQD"
            className="brand-logo brand-logo-cpqd"
          />
        </div>
        <div className="brand-block">
          <div className="brand-line">
            <h1>Machine That Learns</h1>
          </div>
        </div>
        <div className="logo-slot logo-slot-right">
          <img
            src="https://postech.fiap.com.br/svg/fiap-plus-alura.svg"
            alt="FIAP"
            className="brand-logo brand-logo-fiap"
          />
        </div>
      </div>
      <div className="badge-row badge-row-centered">
        <article className="metric-pill">
          <span>Amostras</span>
          <strong>{totalSamples}</strong>
        </article>
        <article className="metric-pill">
          <span>Classes prontas</span>
          <strong>
            {readyClasses}/{classCount}
          </strong>
        </article>
        <article className="metric-pill">
          <span>Modo</span>
          <strong>{isPredicting ? 'Ao vivo' : 'Coleta'}</strong>
        </article>
      </div>
    </section>
  )
}
