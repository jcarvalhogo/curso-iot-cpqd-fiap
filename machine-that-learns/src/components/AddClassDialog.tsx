import type { FormEvent } from 'react'

type AddClassDialogProps = {
  isOpen: boolean
  className: string
  onClose: () => void
  onChangeClassName: (value: string) => void
  onSubmit: (event: FormEvent<HTMLFormElement>) => void
}

export function AddClassDialog({
  isOpen,
  className,
  onClose,
  onChangeClassName,
  onSubmit,
}: AddClassDialogProps) {
  if (!isOpen) {
    return null
  }

  return (
    <div className="modal-backdrop" onClick={onClose}>
      <dialog className="class-dialog" open onClick={(event) => event.stopPropagation()}>
        <form onSubmit={onSubmit} className="dialog-form">
          <div>
            <p className="eyebrow">Nova classe</p>
            <h3>Adicionar classe para teste</h3>
          </div>
          <label htmlFor="new-class-name">Nome da classe</label>
          <input
            id="new-class-name"
            type="text"
            value={className}
            onChange={(event) => onChangeClassName(event.target.value)}
            autoFocus
          />
          <div className="dialog-actions">
            <button type="button" className="button-secondary" onClick={onClose}>
              Cancelar
            </button>
            <button type="submit" className="button-primary">
              Criar classe
            </button>
          </div>
        </form>
      </dialog>
    </div>
  )
}
