import React, { useState, useEffect } from "react";
import BlockchainView from "./components/BlockchainView";
import AddTransactionForm from "./components/AddTransactionForm";
import MiningAnimation from "./components/MiningAnimation";
import EditBlockModal from "./components/EditBlockModal";
import UTXOList from "./components/UTXOList";
import "./App.css";

const API_BASE = "http://localhost:8080";

const computeHash = (block) => {
  const data = `${block.index}${block.timestamp}${block.previousHash}${block.nonce}${JSON.stringify(
    block.transactions
  )}`;
  let hash = 0;
  for (let i = 0; i < data.length; i++) {
    const char = data.charCodeAt(i);
    hash = (hash << 5) - hash + char;
    hash |= 0; // 32-bit
  }
  return Math.abs(hash).toString(16).padStart(16, "0");
};

function App() {
  const [blocks, setBlocks] = useState([]);
  const [difficulty, setDifficulty] = useState(0);
  const [balances, setBalances] = useState({});
  const [pending, setPending] = useState([]);
  const [utxos, setUtxos] = useState([]);
  const [loading, setLoading] = useState(false);
  const [banner, setBanner] = useState(null); // { type: "success" | "error", text: string }
  const [tamperMode, setTamperMode] = useState(false);
  const [tamperedBlocks, setTamperedBlocks] = useState(null);
  const [editingBlock, setEditingBlock] = useState(null);
  const [desiredDifficulty, setDesiredDifficulty] = useState("");
  const [miningAttempts, setMiningAttempts] = useState([]);
  const [miningActive, setMiningActive] = useState(false);
  const miningJobRef = React.useRef(null);
  const miningStreamRef = React.useRef(null);
  const [finalHash, setFinalHash] = useState("");

  const fetchChain = async () => {
    const res = await fetch(`${API_BASE}/blockchain`);
    if (!res.ok) {
      throw new Error("Failed to fetch blockchain");
    }
    return res.json();
  };

  const fetchBalances = async () => {
    const res = await fetch(`${API_BASE}/balances`);
    if (!res.ok) {
      throw new Error("Failed to fetch balances");
    }
    return res.json();
  };

  const fetchPending = async () => {
    const res = await fetch(`${API_BASE}/pending`);
    if (!res.ok) {
      throw new Error("Failed to fetch pending transactions");
    }
    return res.json();
  };

  const fetchUTXOs = async () => {
    const res = await fetch(`${API_BASE}/utxos`);
    if (!res.ok) {
      throw new Error("Failed to fetch UTXO set");
    }
    return res.json();
  };

  const calculateValidity = (chain) => {
    const result = [];
    let broken = false;
    for (let i = 0; i < chain.length; i++) {
      const block = chain[i];
      const prev = chain[i - 1];
      const tampered = block.__tampered === true;
      const linkOk = i === 0 || block.previousHash === prev.hash;
      broken = broken || tampered || !linkOk;
      const valid = !broken;
      result.push({ ...block, isValid: valid });
    }
    return result;
  };

  const refresh = async () => {
    let chainData = null;
    try {
      setLoading(true);
      const [cData, balanceData, pendingData, utxoData] = await Promise.all([
        fetchChain(),
        fetchBalances(),
        fetchPending(),
        fetchUTXOs(),
      ]);
      chainData = cData;
      setBlocks(cData.chain || []);
      setDifficulty(cData.difficulty || 0);
      setBalances(balanceData || {});
      setPending(pendingData || []);
      setUtxos(utxoData || []);
      setBanner(null);
      setTamperedBlocks(null);
      setTamperMode(false);
    } catch (err) {
      setBanner({ type: "error", text: err.message });
    }
    setLoading(false);
    return chainData;
  };

  useEffect(() => {
    refresh();
  }, []);

  useEffect(() => {
    return () => {
      if (miningStreamRef.current) {
        miningStreamRef.current.close();
        miningStreamRef.current = null;
      }
    };
  }, []);

  const handleAddTransaction = async (tx) => {
    try {
      setLoading(true);
      const res = await fetch(`${API_BASE}/transaction`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(tx),
      });
      const data = await res.json();
      if (!res.ok || data.status !== "success") {
        throw new Error(data.message || "Transaction failed");
      }
      setBanner({ type: "success", text: "✅ Transaction accepted" });
      await refresh();
    } catch (err) {
      setBanner({ type: "error", text: err.message });
    } finally {
      setLoading(false);
    }
  };

  const stopMiningStream = ({ clearAttempts = true, deactivate = true } = {}) => {
    if (miningStreamRef.current) {
      miningStreamRef.current.close();
      miningStreamRef.current = null;
    }
    if (clearAttempts) {
      setMiningAttempts([]);
    }
    miningJobRef.current = null;
    if (deactivate) {
      setMiningActive(false);
    }
  };

  const handleMine = async (miner = "default_miner") => {
    try {
      setMiningActive(true);
      setMiningAttempts([]);
      setBanner(null);
      if (miningStreamRef.current) {
        miningStreamRef.current.close();
        miningStreamRef.current = null;
      }
      const res = await fetch(`${API_BASE}/mine/start`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ miner }),
      });
      const data = await res.json();
      if (!res.ok || data.status !== "started") {
        throw new Error(data.message || "Mining start failed");
      }
      miningJobRef.current = data.jobId;

      const stream = new EventSource(`${API_BASE}/mine/stream?id=${data.jobId}`);
      miningStreamRef.current = stream;

      stream.onmessage = async (evt) => {
        try {
          const status = JSON.parse(evt.data);
          if (Array.isArray(status.attempts)) {
            setMiningAttempts(
              status.attempts.map((h, idx) => ({
                hash: h.includes(":") ? h.split(":")[1] : h,
                nonce: h.includes(":") ? h.split(":")[0] : "",
                success: status.status === "done" && idx === status.attempts.length - 1,
              }))
            );
          }
          if (status.status === "done") {
            if (miningStreamRef.current) {
              miningStreamRef.current.onerror = null; // avoid error firing after close
            }
            setBanner({
              type: "success",
              text: `⛏️ Block mined! Hash ${status.hash?.slice(0, 12) || ""}...`,
            });
            await refresh();
            setFinalHash(status.hash || "");
            stopMiningStream({ clearAttempts: false, deactivate: false });
            setTimeout(() => {
              setMiningActive(false);
              setMiningAttempts([]);
              setFinalHash("");
            }, 3000);
          } else if (status.status === "error") {
            throw new Error(status.message || "Mining failed");
          }
        } catch (err) {
          setBanner({ type: "error", text: err.message });
          stopMiningStream({ clearAttempts: false });
        }
      };

      stream.onerror = () => {
        if (stream.readyState === EventSource.CLOSED) return;
        setBanner({ type: "error", text: "Mining stream disconnected" });
        stopMiningStream({ clearAttempts: false });
      };
    } catch (err) {
      setBanner({ type: "error", text: err.message });
      setMiningActive(false);
    }
  };

  const handleSetDifficulty = async () => {
    const value = parseInt(desiredDifficulty, 10);
    if (Number.isNaN(value) || value < 1) {
      setBanner({ type: "error", text: "난이도는 1 이상의 정수여야 합니다." });
      return;
    }
    try {
      setLoading(true);
      const res = await fetch(`${API_BASE}/difficulty`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ difficulty: value }),
      });
      const data = await res.json();
      if (!res.ok || data.status !== "success") {
        throw new Error(data.message || "Difficulty update failed");
      }
      setBanner({ type: "success", text: `⚙️ 난이도 변경: ${value}` });
      await refresh();
    } catch (err) {
      setBanner({ type: "error", text: err.message });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div style={{ padding: "2rem", maxWidth: "1200px", margin: "0 auto" }}>
      <h1>🔗 ToyChain Explorer</h1>

      {banner && (
        <div
          style={{
            backgroundColor: banner.type === "error" ? "#fef2f2" : "#ecfdf3",
            border: banner.type === "error" ? "2px solid #ef4444" : "2px solid #34d399",
            borderRadius: "0.5rem",
            padding: "1rem",
            marginBottom: "1.5rem",
            color: banner.type === "error" ? "#b91c1c" : "#047857",
          }}
        >
          {banner.text}
        </div>
      )}

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
        </div>
        <div style={{ marginTop: "1rem", display: "flex", gap: "0.5rem", flexWrap: "wrap" }}>
          <input
            type="number"
            min="1"
            placeholder="난이도 입력"
            value={desiredDifficulty}
            onChange={(e) => setDesiredDifficulty(e.target.value)}
            style={{ padding: "0.5rem", borderRadius: "6px", border: "1px solid #cbd5e1", width: "140px" }}
          />
          <button className="validate-btn" onClick={handleSetDifficulty} disabled={loading}>
            ⚙️ Set Difficulty
          </button>
        </div>
      </div>

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
          <p style={{ margin: 0, color: "#6b7280" }}>
            No balances yet. Mine a block to create coinbase UTXOs.
          </p>
        )}
      </div>

      <div
        style={{
          backgroundColor: "#f8fafc",
          border: "2px solid #cbd5e1",
          borderRadius: "0.5rem",
          padding: "1.5rem",
          marginBottom: "2rem",
        }}
      >
        <h3 style={{ margin: "0 0 1rem 0" }}>🧾 UTXO Set</h3>
        <UTXOList utxos={utxos} />
      </div>

      <div className="controls">
        <AddTransactionForm onAdd={handleAddTransaction} onMine={handleMine} />
        <button className="validate-btn" onClick={refresh} disabled={loading}>
          🔄 Refresh
        </button>
        <button
          className="validate-btn"
          onClick={() => {
            setTamperMode((prev) => !prev);
            setTamperedBlocks((prev) => (prev ? null : blocks.map((b) => ({ ...b }))));
            setEditingBlock(null);
          }}
        >
          {tamperMode ? "🔒 Exit Tamper Mode" : "💣 Tamper Mode"}
        </button>
      </div>

      {loading && (
        <p style={{ color: "#64748b" }}>⏳ Syncing with node...</p>
      )}

      {miningActive && (
        <MiningAnimation attempts={miningAttempts} difficulty={difficulty} finalHash={finalHash} />
      )}

      <BlockchainView
        blocks={calculateValidity(tamperedBlocks || blocks)}
        pendingTransactions={pending}
        tamperMode={tamperMode}
        onEditBlock={(idx) => setEditingBlock(idx)}
      />

      {editingBlock !== null && tamperedBlocks && (
        <EditBlockModal
          block={tamperedBlocks[editingBlock]}
          onSave={(newBlock) => {
            const tamperedHash = computeHash(newBlock);
            const next = tamperedBlocks.map((b, i) =>
              i === editingBlock ? { ...newBlock, __tampered: true, hash: tamperedHash } : b
            );
            setTamperedBlocks(next);
            setEditingBlock(null);
          }}
          onClose={() => setEditingBlock(null)}
        />
      )}
    </div>
  );
}

export default App;
