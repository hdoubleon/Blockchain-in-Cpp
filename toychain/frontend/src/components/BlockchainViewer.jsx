import React from "react";

function BlockchainViewer({ blocks }) {
  const isBlockValid = (block, index) => {
    if (index === 0) return true; // Genesis block
    const prevBlock = blocks[index - 1];
    return block.previousHash === prevBlock.hash;
  };

  const checkHashDifficulty = (hash, difficulty) => {
    const prefix = "0".repeat(difficulty);
    return hash.startsWith(prefix);
  };

  return (
    <div>
      <h2>📦 Blockchain</h2>
      {blocks.map((block, index) => {
        const isValid = isBlockValid(block, index);
        const hashValid = checkHashDifficulty(block.hash, block.difficulty || 2);

        return (
          <div
            key={block.index}
            style={{
              border: `2px solid ${isValid && hashValid ? "#10b981" : "#ef4444"}`,
              borderRadius: "0.5rem",
              padding: "1rem",
              marginBottom: "1rem",
              backgroundColor: isValid && hashValid ? "#f0fdf4" : "#fef2f2",
              position: "relative",
            }}
          >
            {(!isValid || !hashValid) && (
              <div
                style={{
                  position: "absolute",
                  top: "0.5rem",
                  right: "0.5rem",
                  backgroundColor: "#ef4444",
                  color: "white",
                  padding: "0.25rem 0.75rem",
                  borderRadius: "0.25rem",
                  fontSize: "0.875rem",
                  fontWeight: "600",
                }}
              >
                ⚠️ INVALID
              </div>
            )}
            <h3>Block #{block.index}</h3>
            <div style={{ display: "grid", gap: "0.5rem" }}>
              <p>
                <strong>Timestamp:</strong> {new Date(block.timestamp).toLocaleString()}
              </p>
              <p style={{ wordBreak: "break-all" }}>
                <strong>Hash:</strong> <code>{block.hash}</code>
              </p>
              <p style={{ wordBreak: "break-all" }}>
                <strong>Previous Hash:</strong> <code>{block.previousHash}</code>
              </p>
              <p>
                <strong>Nonce:</strong> {block.nonce}
              </p>
              <p>
                <strong>Difficulty:</strong>
                <span style={{
                  marginLeft: "0.5rem",
                  padding: "0.125rem 0.5rem",
                  backgroundColor: hashValid ? "#10b981" : "#ef4444",
                  color: "white",
                  borderRadius: "0.25rem",
                  fontSize: "0.875rem"
                }}>
                  {block.difficulty || 2}
                </span>
              </p>
              <div>
                <strong>Transactions:</strong>
                {block.transactions?.length > 0 ? (
                  <ul style={{ marginTop: "0.5rem" }}>
                    {block.transactions.map((tx, i) => (
                      <li key={i}>
                        {tx.sender} → {tx.recipient}: <strong>{tx.amount}</strong> coins
                      </li>
                    ))}
                  </ul>
                ) : (
                  <p style={{ marginLeft: "1rem", color: "#6b7280" }}>Genesis block</p>
                )}
              </div>
            </div>
            {index < blocks.length - 1 && (
              <div style={{ textAlign: "center", margin: "1rem 0" }}>
                <span style={{ fontSize: "2rem" }}>
                  {isValid ? "⬇️" : "❌"}
                </span>
              </div>
            )}
          </div>
        );
      })}
    </div>
  );
}

export default BlockchainViewer;
