import React, { useState } from "react";

function AddTransactionForm({ onAdd, onMine }) {
  const [sender, setSender] = useState("");
  const [recipient, setRecipient] = useState("");
  const [amount, setAmount] = useState("");

  const handleSubmit = (event) => {
    event.preventDefault();
    if (!sender || !recipient || !amount) {
      return;
    }

    onAdd({
      sender,
      recipient,
      amount: Number.parseFloat(amount) || 0,
    });

    setSender("");
    setRecipient("");
    setAmount("");
  };

  return (
    <form onSubmit={handleSubmit} style={{ marginBottom: "2rem" }}>
      <h2>Add Transaction</h2>
      <div style={{ display: "flex", gap: "1rem", flexWrap: "wrap" }}>
        <label>
          Sender
          <input
            type="text"
            value={sender}
            onChange={(e) => setSender(e.target.value)}
            required
            style={{ marginLeft: "0.5rem" }}
          />
        </label>
        <label>
          Recipient
          <input
            type="text"
            value={recipient}
            onChange={(e) => setRecipient(e.target.value)}
            required
            style={{ marginLeft: "0.5rem" }}
          />
        </label>
        <label>
          Amount
          <input
            type="number"
            min="0"
            step="0.01"
            value={amount}
            onChange={(e) => setAmount(e.target.value)}
            required
            style={{ marginLeft: "0.5rem" }}
          />
        </label>
      </div>
      <div style={{ marginTop: "1.5rem", display: "flex", gap: "1rem" }}>
        <button
          type="submit"
          style={{
            padding: "0.75rem 2rem",
            fontSize: "1rem",
            fontWeight: "600",
            color: "white",
            backgroundColor: "#3b82f6",
            border: "none",
            borderRadius: "0.5rem",
            cursor: "pointer",
            transition: "all 0.2s ease",
            boxShadow: "0 2px 4px rgba(59, 130, 246, 0.3)",
          }}
          onMouseOver={(e) => e.target.style.backgroundColor = "#2563eb"}
          onMouseOut={(e) => e.target.style.backgroundColor = "#3b82f6"}
        >
          ✓ Add Transaction
        </button>
        <button
          type="button"
          onClick={() => onMine()}
          style={{
            padding: "0.75rem 2rem",
            fontSize: "1rem",
            fontWeight: "600",
            color: "white",
            backgroundColor: "#10b981",
            border: "none",
            borderRadius: "0.5rem",
            cursor: "pointer",
            transition: "all 0.2s ease",
            boxShadow: "0 2px 4px rgba(16, 185, 129, 0.3)",
          }}
          onMouseOver={(e) => e.target.style.backgroundColor = "#059669"}
          onMouseOut={(e) => e.target.style.backgroundColor = "#10b981"}
        >
          ⛏️ Mine Block
        </button>
      </div>
    </form>
  );
}

export default AddTransactionForm;
