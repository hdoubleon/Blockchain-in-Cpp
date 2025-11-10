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
      <div style={{ marginTop: "1rem", display: "flex", gap: "1rem" }}>
        <button type="submit">Add</button>
        <button type="button" onClick={onMine}>
          Mine Block
        </button>
      </div>
    </form>
  );
}

export default AddTransactionForm;
