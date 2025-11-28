import React from "react";
import "./BlockchainView.css";

function BlockchainView({ blocks, pendingTransactions = [], tamperMode = false, onEditBlock }) {
  return (
    <div className="blockchain-container">
      {pendingTransactions.length > 0 && (
        <div className="pending-section">
          <h2>⏳ Pending Transactions ({pendingTransactions.length})</h2>
          <div className="pending-box">
            {pendingTransactions.map((tx, i) => (
              <div key={i} className="pending-tx">
                <div style={{ fontWeight: 600 }}>
                  TX {tx.id.slice(0, 12)}...
                </div>
                <div style={{ fontSize: "0.9rem", color: "#475569" }}>
                  {tx.inputs.length === 0 ? "coinbase" : `${tx.inputs.length} inputs`} → {tx.outputs.length} outputs
                </div>
              </div>
            ))}
          </div>
        </div>
      )}

      <h2>⛓️ Blockchain</h2>
      <div className="chain">
        {blocks.map((block, index) => {
          const nextBroken = blocks[index + 1] && !blocks[index + 1].isValid;
          return (
          <React.Fragment key={block.index}>
            <div className={`block ${block.isValid ? "valid" : "invalid"}`}>
              <div className="block-header">
                <span className="block-index">Block #{block.index}</span>
                <span className={`status ${block.isValid ? "valid" : "invalid"}`}>
                  {block.isValid ? "✅ Valid" : "❌ Broken"}
                </span>
              </div>

              {!block.isValid && <div className="block-break-overlay">CHAIN BROKEN</div>}

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
                  <strong>Difficulty:</strong> {block.difficulty}
                </div>
                <div className="block-field">
                  <strong>Timestamp:</strong> {block.timestamp}
                </div>

                <details>
                  <summary>Transactions ({block.transactions.length})</summary>
                  {block.transactions.map((tx, i) => {
                    const isCoinbase = tx.inputs.length === 0;
                    return (
                      <div key={i} className="transaction">
                        <div style={{ fontWeight: 600 }}>
                          TX {tx.id.slice(0, 12)}...
                        </div>
                        <div style={{ fontSize: "0.9rem", color: "#475569" }}>
                          {isCoinbase ? "coinbase" : `${tx.inputs.length} inputs`} → {tx.outputs.length} outputs
                        </div>
                        <div className="tx-section">
                          <strong>Inputs:</strong>{" "}
                          {isCoinbase ? (
                            <span>— (block reward)</span>
                          ) : (
                            tx.inputs.map((input, idx) => (
                              <div key={idx} className="tx-io">
                                {input.txId.slice(0, 10)}...:{input.outputIndex} ({input.signature})
                              </div>
                            ))
                          )}
                        </div>
                        <div className="tx-section">
                          <strong>Outputs:</strong>
                          {tx.outputs.map((output, idx) => (
                            <div key={idx} className="tx-io">
                              {output.address} — {output.amount}
                            </div>
                          ))}
                        </div>
                      </div>
                    );
                  })}
                </details>
              </div>

              {tamperMode && block.index > 0 && (
                <button className="tamper-btn" onClick={() => onEditBlock && onEditBlock(index)}>
                  ✏️ Tamper Transactions
                </button>
              )}
            </div>

            {index < blocks.length - 1 && (
              <div className={`chain-link ${nextBroken ? "broken" : "valid"}`}>
                <div className="link-line"></div>
                {nextBroken && <span className="break-label">⚡ Link Broken</span>}
              </div>
            )}
          </React.Fragment>
        );
        })}
      </div>
    </div>
  );
}

export default BlockchainView;
