import React from "react";

const cardStyle = {
  border: "1px solid #ccc",
  borderRadius: 8,
  padding: "1rem",
  marginBottom: "1rem",
  background: "#fafafa",
};

function BlockCard({ block }) {
  return (
    <div style={cardStyle}>
      <h3>Block #{block.index}</h3>
      <p>
        <strong>Timestamp:</strong> {block.timestamp}
      </p>
      <p>
        <strong>Hash:</strong> <code>{block.hash}</code>
      </p>
      <p>
        <strong>Previous Hash:</strong> <code>{block.previousHash}</code>
      </p>
      <div>
        <strong>Transactions</strong>
        <ul>
          {block.transactions.map((tx, index) => (
            <li key={index}>
              {tx.sender} → {tx.recipient} ({tx.amount})
            </li>
          ))}
        </ul>
      </div>
    </div>
  );
}

export default BlockCard;
