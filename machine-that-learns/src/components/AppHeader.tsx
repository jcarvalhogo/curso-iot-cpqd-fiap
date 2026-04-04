import cpqdLettersWhite from '../assets/cpqd-letters-white.svg'

export function AppHeader() {
  return (
    <section className="topbar">
      <div className="brand-header">
        <div className="logo-slot logo-slot-left">
          <div className="cpqd-logo-lockup" aria-label="CPQD" role="img">
            <img
              src="https://d3dicilwvhavgk.cloudfront.net/cpqd-vlec-public/productcustom/logologin/638048833004891759/default.png"
              alt=""
              aria-hidden="true"
              className="brand-logo brand-logo-cpqd-base"
            />
            <img
              src={cpqdLettersWhite}
              alt=""
              aria-hidden="true"
              className="brand-logo brand-logo-cpqd-overlay"
            />
          </div>
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
    </section>
  )
}
