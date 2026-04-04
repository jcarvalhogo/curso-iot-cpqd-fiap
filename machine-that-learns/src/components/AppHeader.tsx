import cpqdLettersWhite from '../assets/cpqd-letters-white.svg'

export function AppHeader() {
  return (
    <section className="topbar">
      <div className="brand-header">
        <div className="logo-slot logo-slot-left">
          <a
            href="https://www.cpqd.com.br/"
            target="_blank"
            rel="noreferrer"
            className="logo-link"
            aria-label="Abrir site do CPQD"
          >
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
          </a>
        </div>
        <div className="brand-block">
          <div className="brand-line">
            <h1>Machine That Learns</h1>
          </div>
        </div>
        <div className="logo-slot logo-slot-right">
          <a
            href="https://postech.fiap.com.br/?utm_term=fiap%20pos%20gradua%C3%A7%C3%A3o&utm_campaign=PSQ+-+LP+-+Pos+Grad+-+Institucional&utm_source=adwords&utm_medium=ppc&hsa_acc=8723308225&hsa_cam=19460817008&hsa_grp=147674571969&hsa_ad=674665849807&hsa_src=g&hsa_tgt=kwd-365662428662&hsa_kw=fiap%20pos%20gradua%C3%A7%C3%A3o&hsa_mt=b&hsa_net=adwords&hsa_ver=3&gad_source=1&gad_campaignid=19460817008&gbraid=0AAAAApIIu6N7ikYWhSgmmcqUE5VKtU9_n&gclid=Cj0KCQjw7cLOBhDmARIsAGsuA0kvuZLwkqqyiiFso_9ugKe7ztQvaeU1EuFvck3J90UfIPMusK-TU4oaAi8PEALw_wcB"
            target="_blank"
            rel="noreferrer"
            className="logo-link"
            aria-label="Abrir site da FIAP Alura"
          >
            <img
              src="https://postech.fiap.com.br/svg/fiap-plus-alura.svg"
              alt="FIAP"
              className="brand-logo brand-logo-fiap"
            />
          </a>
        </div>
      </div>
    </section>
  )
}
