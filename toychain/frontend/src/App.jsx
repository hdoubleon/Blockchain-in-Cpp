import React, { useState, useEffect } from "react";
import BlockchainView from "./components/BlockchainView";
import AddTransactionForm from "./components/AddTransactionForm";
import MiningAnimation from "./components/MiningAnimation";
import EditBlockModal from "./components/EditBlockModal";
import "./App.css";

// 제네시스 블록 생성
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

// 아주 단순한 해시 함수 (데모용)
const generateHash = (block) => {
  const data = `${block.index}${block.timestamp}${block.previousHash}${
    block.nonce
  }${JSON.stringify(block.transactions)}`;
  let hash = 0;
  for (let i = 0; i < data.length; i++) {
    const char = data.charCodeAt(i);
    hash = (hash << 5) - hash + char;
    hash = hash & hash;
  }
  return Math.abs(hash).toString(16).padStart(16, "0");
};

function App() {
  // -------------------- state --------------------
  const [blocks, setBlocks] = useState(createInitialChain); // 체인
  const [pendingTransactions, setPendingTransactions] = useState([]); // 아직 포함 안 된 트랜잭션들
  const [difficulty, setDifficulty] = useState(2); // 난이도 (데모용 고정)
  const [balances, setBalances] = useState({}); // 주소별 잔액
  const [isMining, setIsMining] = useState(false); // 마이닝 애니메이션 상태
  const [editingBlock, setEditingBlock] = useState(null); // 수정 중인 블록 인덱스

  // -------------------- 유효성 검증 --------------------
  const getBlocksWithValidity = (blocks) => {
    return blocks.map((block, i, arr) => {
      if (i === 0) return { ...block, isValid: true };

      const expectedHash = generateHash(block);
      const isHashValid = block.hash === expectedHash;
      const isChainValid = block.previousHash === arr[i - 1].hash;

      return { ...block, isValid: isHashValid && isChainValid };
    });
  };

  // -------------------- 잔액 계산 (UTXO스럽게) --------------------
  useEffect(() => {
    const newBalances = {};

    blocks.forEach((block) => {
      block.transactions.forEach((tx) => {
        const { sender, recipient, amount } = tx;

        // 네트워크 리워드 같은 건 sender 차감 X
        if (sender && sender !== "network") {
          newBalances[sender] = (newBalances[sender] || 0) - amount;
        }
        if (recipient) {
          newBalances[recipient] = (newBalances[recipient] || 0) + amount;
        }
      });
    });

    setBalances(newBalances);
  }, [blocks]);

  // -------------------- 트랜잭션 추가 --------------------
  const handleAddTransaction = (tx) => {
    // tx: { sender, recipient, amount } 형태라고 가정
    setPendingTransactions((prev) => [...prev, tx]);
  };

  // -------------------- 마이닝 --------------------
  const handleMine = () => {
    if (pendingTransactions.length === 0) {
      alert("⛏ 추가할 트랜잭션이 없습니다.");
      return;
    }

    setIsMining(true);

    setTimeout(() => {
      setBlocks((prevBlocks) => {
        const lastBlock = prevBlocks[prevBlocks.length - 1];

        // 코인베이스(채굴 보상) 트랜잭션 예시
        const coinbaseTx = {
          sender: "network",
          recipient: "miner",
          amount: 50,
        };

        const newBlock = {
          index: prevBlocks.length,
          timestamp: new Date().toLocaleString(),
          previousHash: lastBlock.hash,
          nonce: 0,
          transactions: [coinbaseTx, ...pendingTransactions],
          hash: "",
          isValid: true,
        };

        // PoW: hash가 0이 difficulty개 연속될 때까지 nonce 증가
        let nonce = 0;
        let hash = "";
        const targetPrefix = "0".repeat(difficulty);

        while (true) {
          const candidate = { ...newBlock, nonce };
          const candidateHash = generateHash(candidate);
          if (candidateHash.startsWith(targetPrefix)) {
            hash = candidateHash;
            newBlock.nonce = nonce;
            newBlock.hash = hash;
            break;
          }
          nonce++;
        }

        return [...prevBlocks, newBlock];
      });

      // 블록에 포함됐으니 pending 트랜잭션 비우기
      setPendingTransactions([]);
      setIsMining(false);
    }, 500); // 살짝 딜레이 줘서 애니메이션 보여주기
  };

  // -------------------- 체인 검증 --------------------
  const validateChain = () => {
    const validated = getBlocksWithValidity(blocks);
    const hasInvalid = validated.some((b) => !b.isValid);
    if (hasInvalid) {
      alert("❌ 체인에 유효하지 않은 블록이 있습니다!");
    } else {
      alert("✅ 체인이 유효합니다!");
    }
  };

  // -------------------- 블록 변조 (EditBlockModal용) --------------------
  const handleTamperBlock = (index, newTransactions) => {
    setBlocks((prevBlocks) => {
      const newBlocks = prevBlocks.map((b) => ({
        ...b,
        transactions: [...b.transactions],
      }));

      const target = newBlocks[index];
      const tamperedBlock = {
        ...target,
        transactions: newTransactions,
      };

      // 변조 후 해시는 다시 계산 (재마이닝은 X)
      tamperedBlock.hash = generateHash(tamperedBlock);
      newBlocks[index] = tamperedBlock;

      return newBlocks;
    });
  };

  // -------------------- JSX --------------------
  return (
    <div style={{ padding: "2rem", maxWidth: "1200px", margin: "0 auto" }}>
      <h1>🔗 ToyChain Explorer</h1>

      {/* 백엔드 안 띄운 경우 안내 (지금은 blocks만 기준으로 간단 체크) */}
      {blocks.length === 0 && (
        <div
          style={{
            backgroundColor: "#fef2f2",
            border: "2px solid #ef4444",
            borderRadius: "0.5rem",
            padding: "1.5rem",
            marginBottom: "2rem",
          }}
        >
          <h3>⚠️ Backend Server Not Running</h3>
          <p>Please start the backend server:</p>
          <pre
            style={{
              backgroundColor: "#1f2937",
              color: "#f3f4f6",
              padding: "1rem",
              borderRadius: "0.25rem",
            }}
          >
            {`cd backend/build
./toychain_server`}
          </pre>
        </div>
      )}

      {/* Difficulty 정보 패널 */}
      <div
        style={{
          backgroundColor: "#f8fafc",
          border: "2px solid #e2e8f0",
          borderRadius: "0.5rem",
          padding: "1.5rem",
          marginBottom: "2rem",
        }}
      >
        <h3 style={{ margin: "0 0 1rem 0" }}>⚙️ Network Stats</h3>
        <div style={{ display: "flex", gap: "2rem", flexWrap: "wrap" }}>
          <div>
            <strong>Current Difficulty:</strong>
            <span
              style={{
                marginLeft: "0.5rem",
                padding: "0.25rem 0.75rem",
                backgroundColor: "#3b82f6",
                color: "white",
                borderRadius: "0.25rem",
                fontWeight: "600",
              }}
            >
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
      <div
        style={{
          backgroundColor: "#f0fdf4",
          border: "2px solid #86efac",
          borderRadius: "0.5rem",
          padding: "1.5rem",
          marginBottom: "2rem",
        }}
      >
        <h3 style={{ margin: "0 0 1rem 0" }}>💰 UTXO Balances</h3>
        {Object.keys(balances).length > 0 ? (
          <div
            style={{
              display: "grid",
              gridTemplateColumns: "repeat(auto-fill, minmax(250px, 1fr))",
              gap: "1rem",
            }}
          >
            {Object.entries(balances).map(([address, balance]) => (
              <div
                key={address}
                style={{
                  backgroundColor: "white",
                  padding: "1rem",
                  borderRadius: "0.375rem",
                  border: "1px solid #86efac",
                }}
              >
                <div
                  style={{ fontWeight: "600", marginBottom: "0.5rem" }}
                >
                  {address}
                </div>
                <div style={{ fontSize: "1.25rem", color: "#10b981" }}>
                  {balance.toFixed(2)} coins
                </div>
              </div>
            ))}
          </div>
        ) : (
          <p style={{ margin: 0, color: "#6b7280" }}>
            No balances yet. Add transactions to see UTXO balances.
          </p>
        )}
      </div>

      {/* 컨트롤 영역 */}
      <div className="controls">
        <AddTransactionForm onAdd={handleAddTransaction} onMine={handleMine} />
        <button className="validate-btn" onClick={validateChain}>
          🔍 Validate Chain
        </button>
      </div>

      {isMining && <MiningAnimation />}

      {/* 블록체인 뷰 */}
      <BlockchainView
        blocks={getBlocksWithValidity(blocks)}
        pendingTransactions={pendingTransactions}
        onEditBlock={(index) => setEditingBlock(index)}
      />

      {/* 블록 수정 모달 */}
      {editingBlock !== null && (
        <EditBlockModal
          block={blocks[editingBlock]}
          onSave={(newTransactions) =>
            handleTamperBlock(editingBlock, newTransactions)
          }
          onClose={() => setEditingBlock(null)}
        />
      )}
    </div>
  );
}

export default App;