import React from "react";
import "./MiningAnimation.css";

function MiningAnimation({ attempts = [], difficulty = 1, finalHash = "" }) {
  const found = finalHash || (attempts.find((a) => a.success) || {}).hash;
  return (
    <div className="mining-overlay">
      <div className="mining-box">
        <div className="block-icon rotating">📦</div>
        <h2>⛏️ Mining Block...</h2>
        {found && (
          <div className="found-banner">
            FOUND: <code>{found}</code>
          </div>
        )}
        <p>Finding hash with {difficulty} leading zero(s)</p>
        <div className="scroll-attempts">
          {attempts.map((a, idx) => (
            <div key={idx} className={`attempt ${a.success ? "success" : ""}`}>
              <code>{a.nonce ? `${a.nonce}` : idx}</code>
              <code>{a.hash}</code>
              <span>{a.success ? "✅ FOUND" : "❌"}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

export default MiningAnimation;
