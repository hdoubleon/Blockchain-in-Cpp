import React, { useState } from "react";
import "./EditBlockModal.css";

function EditBlockModal({ block, onSave, onClose }) {
  const [transactions, setTransactions] = useState(() =>
    block.transactions.map((tx) => ({
      ...tx,
      outputs: tx.outputs.map((o) => ({ ...o })),
      inputs: tx.inputs.map((i) => ({ ...i })),
    }))
  );

  const handleOutputChange = (txIdx, outIdx, value) => {
    const updated = transactions.map((tx, i) => {
      if (i !== txIdx) return tx;
      const outs = tx.outputs.map((o, j) => (j === outIdx ? { ...o, amount: value } : o));
      return { ...tx, outputs: outs };
    });
    setTransactions(updated);
  };

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h2>✏️ Tamper Block #{block.index}</h2>
          <button className="close-btn" onClick={onClose}>
            ✕
          </button>
        </div>

        <div className="modal-body">
          <p className="warning">⚠️ 이 블록을 수정하면 체인이 깨집니다.</p>

          {transactions.map((tx, i) => (
            <div key={i} className="transaction-edit">
              <div className="tx-id">TX {tx.id.slice(0, 12)}...</div>
              <div className="tx-io">
                <strong>Inputs:</strong>{" "}
                {tx.inputs.length === 0 ? "coinbase" : `${tx.inputs.length} inputs`}
              </div>
              <div className="tx-io">
                <strong>Outputs:</strong>
                {tx.outputs.map((out, idx) => (
                  <div key={idx} style={{ display: "flex", gap: "0.5rem", alignItems: "center" }}>
                    <span style={{ width: "120px" }}>{out.address}</span>
                    <input
                      type="number"
                      value={out.amount}
                      onChange={(e) => handleOutputChange(i, idx, Number(e.target.value))}
                      style={{ flex: 1 }}
                    />
                  </div>
                ))}
              </div>
            </div>
          ))}

          <div className="modal-actions">
            <button className="btn-cancel" onClick={onClose}>
              Cancel
            </button>
            <button className="btn-tamper" onClick={() => onSave({ ...block, transactions })}>
              💣 Save Tamper
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}

export default EditBlockModal;
