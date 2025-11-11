import React, { useState } from "react";
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
  const [blocks, setBlocks] = useState(createInitialChain);
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

  return (
    <div className="app-container">
      <header className="app-header">
        <h1>⛓️ ToyChain Explorer</h1>
        <p>Educational Blockchain Simulator</p>
      </header>

      <div className="controls">
        <AddTransactionForm onAdd={handleAddTransaction} onMine={handleMine} />
        <button className="validate-btn" onClick={validateChain}>
          🔍 Validate Chain
        </button>
      </div>

      {isMining && <MiningAnimation />}

      <BlockchainView
        blocks={blocks}
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
