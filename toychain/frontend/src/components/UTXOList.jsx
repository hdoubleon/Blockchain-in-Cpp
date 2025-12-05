import React from "react";

function UTXOList({ utxos = [] }) {
  if (!utxos || utxos.length === 0) {
    return <p style={{ margin: 0, color: "#6b7280" }}>No UTXOs yet. Mine a block or add a transaction.</p>;
  }

  return (
    <div
      style={{
        display: "grid",
        gridTemplateColumns: "repeat(auto-fill, minmax(260px, 1fr))",
        gap: "0.75rem",
      }}
    >
      {utxos.map((utxo) => (
        <div
          key={`${utxo.txId}-${utxo.index}`}
          style={{
            background: "white",
            border: "1px solid #e2e8f0",
            borderRadius: "0.5rem",
            padding: "0.75rem",
            display: "flex",
            flexDirection: "column",
            gap: "0.35rem",
          }}
        >
          <div style={{ fontWeight: 700, color: "#0f172a" }}>
            {utxo.address}
          </div>
          <div style={{ fontSize: "0.9rem", color: "#475569" }}>
            TX {utxo.txId.slice(0, 12)}... • #{utxo.index}
          </div>
          <div style={{ fontSize: "1.1rem", color: "#059669", fontWeight: 700 }}>
            {Number(utxo.amount).toFixed(2)} coins
          </div>
        </div>
      ))}
    </div>
  );
}

export default UTXOList;
