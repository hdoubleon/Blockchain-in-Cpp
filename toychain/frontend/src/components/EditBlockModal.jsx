import React, { useState } from "react";
import "./EditBlockModal.css";

function EditBlockModal({ block, onSave, onClose }) {
  const [transactions, setTransactions] = useState([...block.transactions]);

  const handleChangeTransaction = (index, field, value) => {
    const updated = [...transactions];
    updated[index] = { ...updated[index], [field]: value };
    setTransactions(updated);
  };

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h2>💣 Tamper Block #{block.index}</h2>
          <button className="close-btn" onClick={onClose}>✕</button>
        </div>

        <div className="modal-body">
          <p className="warning">
            ⚠️ Warning: Modifying this block will invalidate the entire chain!
          </p>

          <h3>Transactions:</h3>
          {transactions.map((tx, i) => (
            <div key={i} className="transaction-edit">
              <label>
                Sender:
                <input
                  type="text"
                  value={tx.sender}
                  onChange={(e) => handleChangeTransaction(i, "sender", e.target.value)}
                />
              </label>
              <label>
                Recipient:
                <input
                  type="text"
                  value={tx.recipient}
                  onChange={(e) => handleChangeTransaction(i, "recipient", e.target.value)}
                />
              </label>
              <label>
                Amount:
                <input
                  type="number"
                  value={tx.amount}
                  onChange={(e) => handleChangeTransaction(i, "amount", Number(e.target.value))}
                />
              </label>
            </div>
          ))}

          <div className="modal-actions">
            <button className="btn-cancel" onClick={onClose}>Cancel</button>
            <button className="btn-tamper" onClick={() => onSave(transactions)}>
              💣 Tamper Block
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}

export default EditBlockModal;