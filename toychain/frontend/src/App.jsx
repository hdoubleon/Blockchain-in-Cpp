import React, { useState, useEffect } from "react";
import BlockchainView from "./components/BlockchainView";
import AddTransactionForm from "./components/AddTransactionForm";
import MiningAnimation from "./components/MiningAnimation";
import EditBlockModal from "./components/EditBlockModal";
import "./App.css";

const createInitialChain = () => [
  {
    index: 0,
    timestamp: "Genesis Block",
    hash: "0",
    previousHash: "0",
    nonce: 0,
    transactions: [{ sender: "network", recipient: "genesis", amount: 0 }],
    isValid: true,
  },
];

const generateHash = (block) => {
  const data = `${block.index}${block.timestamp}${block.previousHash}${block.nonce}${JSON.stringify(block.transactions)}`;
  let hash = 0;
  for (let i = 0; i < data.length; i++) {
    const char = data.charCodeAt(i);
    hash = (hash << 5) - hash + char;
    hash = hash & hash;
  }
  return Math.abs(hash).toString(16).padStart(16, "0");
};

function App() {
  const [blocks, setBlocks] = useState([]);
  const [difficulty, setDifficulty] = useState(2);
  const [balances, setBalances] = useState({});
  const [pendingTransactions, setPendingTransactions] = useState([]);
  const [isMining, setIsMining] = useState(false);
  const [editingBlock, setEditingBlock] = useState(null);

  const handleAddTransaction = (transaction) => {
    setPendingTransactions((prev) => [...prev, transaction]);
  };

  const handleMine = () => {
    if (!pendingTransactions.length || isMining) {
      return;
    }

    setIsMining(true);

    // 마이닝 애니메이션 시뮬레이션 (2초)
    setTimeout(() => {
      const previousBlock = blocks[blocks.length - 1];
      const newBlock = {
        index: blocks.length,
        timestamp: new Date().toLocaleString(),
        previousHash: previousBlock.hash,
        nonce: 0,
        transactions: [
          ...pendingTransactions,
          { sender: "network", recipient: "miner", amount: 10 },
        ],
        isValid: true,
      };

      // 간단한 PoW 시뮬레이션 (난이도 2)
      let hash = "";
      while (!hash.startsWith("00")) {
        newBlock.nonce++;
        hash = generateHash(newBlock);
      }
      newBlock.hash = hash;

      setBlocks((prev) => [...prev, newBlock]);
      setPendingTransactions([]);
      setIsMining(false);
    }, 2000);
  };

  const handleTamperBlock = (blockIndex, newTransactions) => {
    setBlocks((prev) => {
      const updated = [...prev];
      updated[blockIndex] = {
        ...updated[blockIndex],
        transactions: newTransactions,
        hash: generateHash({ ...updated[blockIndex], transactions: newTransactions }),
      };

      // 체인 검증 (변조된 블록 이후 모두 무효화)
      for (let i = blockIndex; i < updated.length; i++) {
        if (i === 0) continue;
        
        const isHashValid = updated[i].hash === generateHash(updated[i]);
        const isChainValid = updated[i].previousHash === updated[i - 1].hash;
        updated[i].isValid = isHashValid && isChainValid;
        
        if (i > blockIndex) {
          updated[i].isValid = false; // 후속 블록 모두 무효
        }
      }

      return updated;
    });
    setEditingBlock(null);
  };

  const validateChain = () => {
    setBlocks((prev) => {
      const updated = [...prev];
      for (let i = 1; i < updated.length; i++) {
        const isHashValid = updated[i].hash === generateHash(updated[i]);
        const isChainValid = updated[i].previousHash === updated[i - 1].hash;
        updated[i].isValid = isHashValid && isChainValid;
      }
      return updated;
    });
  };

  const fetchBlockchain = async () => {
    try {
      const response = await fetch("http://localhost:8080/blockchain");
      const data = await response.json();
      setBlocks(data.chain || []);
      setDifficulty(data.difficulty || 2);
    } catch (error) {
      console.error("Failed to fetch blockchain:", error);
    }
  };

  const fetchBalances = async () => {
    try {
      const response = await fetch("http://localhost:8080/balances");
      const data = await response.json();
      setBalances(data || {});
    } catch (error) {
      console.error("Failed to fetch balances:", error);
    }
  };

  useEffect(() => {
    fetchBlockchain();
    fetchBalances();
    const interval = setInterval(() => {
      fetchBlockchain();
      fetchBalances();
    }, 3000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div style={{ padding: "2rem", maxWidth: "1200px", margin: "0 auto" }}>
      <h1>🔗 ToyChain Explorer</h1>
      
      {blocks.length === 0 && (
        <div style={{
          backgroundColor: "#fef2f2",
          border: "2px solid #ef4444",
          borderRadius: "0.5rem",
          padding: "1.5rem",
          marginBottom: "2rem",
        }}>
          <h3>⚠️ Backend Server Not Running</h3>
          <p>Please start the backend server:</p>
          <pre style={{ backgroundColor: "#1f2937", color: "#f3f4f6", padding: "1rem", borderRadius: "0.25rem" }}>
{`cd backend/build
./toychain_server`}
          </pre>
        </div>
      )}
      
      {/* Difficulty 정보 패널 */}
      <div style={{
        backgroundColor: "#f8fafc",
        border: "2px solid #e2e8f0",
        borderRadius: "0.5rem",
        padding: "1.5rem",
        marginBottom: "2rem",
      }}>
        <h3 style={{ margin: "0 0 1rem 0" }}>⚙️ Network Stats</h3>
        <div style={{ display: "flex", gap: "2rem", flexWrap: "wrap" }}>
          <div>
            <strong>Current Difficulty:</strong> 
            <span style={{ 
              marginLeft: "0.5rem",
              padding: "0.25rem 0.75rem",
              backgroundColor: "#3b82f6",
              color: "white",
              borderRadius: "0.25rem",
              fontWeight: "600"
            }}>
              {difficulty}
            </span>
          </div>
          <div>
            <strong>Total Blocks:</strong> {blocks.length}
          </div>
          <div>
            <strong>Target Block Time:</strong> 10s
          </div>
        </div>
      </div>

      {/* UTXO 잔액 패널 */}
      <div style={{
        backgroundColor: "#f0fdf4",
        border: "2px solid #86efac",
        borderRadius: "0.5rem",
        padding: "1.5rem",
        marginBottom: "2rem",
      }}>
        <h3 style={{ margin: "0 0 1rem 0" }}>💰 UTXO Balances</h3>
        {Object.keys(balances).length > 0 ? (
          <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(250px, 1fr))", gap: "1rem" }}>
            {Object.entries(balances).map(([address, balance]) => (
              <div key={address} style={{
                backgroundColor: "white",
                padding: "1rem",
                borderRadius: "0.375rem",
                border: "1px solid #86efac"
              }}>
                <div style={{ fontWeight: "600", marginBottom: "0.5rem" }}>
                  {address}
                </div>
                <div style={{ fontSize: "1.25rem", color: "#10b981" }}>
                  {balance.toFixed(2)} coins
                </div>
              </div>
            ))}
          </div>
        ) : (
          <p style={{ margin: 0, color: "#6b7280" }}>No balances yet. Add transactions to see UTXO balances.</p>
        )}
      </div>

      <div className="controls">
        <AddTransactionForm onAdd={handleAddTransaction} onMine={handleMine} />
        <button className="validate-btn" onClick={validateChain}>
          🔍 Validate Chain
        </button>
      </div>

      {isMining && <MiningAnimation />}

      <BlockchainView
        blocks={blocks.map((block, i) =>
        i === 0 ? { ...block, isValid: true } : block
        )}
        pendingTransactions={pendingTransactions}
        onEditBlock={(index) => setEditingBlock(index)}
      />

      {editingBlock !== null && (
        <EditBlockModal
          block={blocks[editingBlock]}
          onSave={(newTransactions) => handleTamperBlock(editingBlock, newTransactions)}
          onClose={() => setEditingBlock(null)}
        />
      )}
    </div>
  );
}

export default App;
