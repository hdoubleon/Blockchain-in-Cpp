import React from "react";
import "./BlockchainView.css";

function BlockchainView({ blocks, pendingTransactions, onEditBlock }) {
  return (
    <div className="blockchain-container">
      {pendingTransactions.length > 0 && (
        <div className="pending-section">
          <h2>⏳ Pending Transactions ({pendingTransactions.length})</h2>
          <div className="pending-box">
            {pendingTransactions.map((tx, i) => (
              <div key={i} className="pending-tx">
                {tx.sender} → {tx.recipient}: {tx.amount} BTC
              </div>
            ))}
          </div>
        </div>
      )}

      <h2>⛓️ Blockchain</h2>
      <div className="chain">
        {blocks.map((block, index) => (
          <React.Fragment key={block.index}>
            <div className={`block ${block.isValid ? "valid" : "invalid"}`}>
              <div className="block-header">
                <span className="block-index">Block #{block.index}</span>
                {block.isValid ? (
                  <span className="status valid">✅ Valid</span>
                ) : (
                  <span className="status invalid">❌ Invalid</span>
                )}
              </div>

              <div className="block-body">
                <div className="block-field">
                  <strong>Hash:</strong>
                  <code className="hash">{block.hash.slice(0, 16)}...</code>
                </div>
                <div className="block-field">
                  <strong>Previous:</strong>
                  <code className="hash">{block.previousHash.slice(0, 16)}...</code>
                </div>
                <div className="block-field">
                  <strong>Nonce:</strong> {block.nonce}
                </div>
                <div className="block-field">
                  <strong>Timestamp:</strong> {block.timestamp}
                </div>

                <details>
                  <summary>Transactions ({block.transactions.length})</summary>
                  {block.transactions.map((tx, i) => (
                    <div key={i} className="transaction">
                      {tx.sender} → {tx.recipient}: {tx.amount}
                    </div>
                  ))}
                </details>
              </div>

              {block.index > 0 && (
                <button className="tamper-btn" onClick={() => onEditBlock(index)}>
                  💣 Tamper
                </button>
              )}
            </div>

            {index < blocks.length - 1 && (
              <div className={`chain-link ${blocks[index + 1].isValid ? "valid" : "broken"}`}>
                <div className="link-line"></div>
                {!blocks[index + 1].isValid && <span className="break-label">⚠️ BROKEN</span>}
              </div>
            )}
          </React.Fragment>
        ))}
      </div>
    </div>
  );
}

export default BlockchainView;
