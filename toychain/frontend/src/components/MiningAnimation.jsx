import React from "react";
import "./MiningAnimation.css";

function MiningAnimation() {
  return (
    <div className="mining-overlay">
      <div className="mining-box">
        <div className="block-icon rotating">📦</div>
        <h2>⛏️ Mining Block...</h2>
        <div className="progress-bar">
          <div className="progress-fill"></div>
        </div>
        <p>Finding valid hash with nonce...</p>
        <div className="hash-attempts">
          <code>Attempt: 0000a3f9... ❌</code>
          <code>Attempt: 0000b7d4... ❌</code>
          <code>Attempt: 00c2e5f0... ❌</code>
          <code className="success">Attempt: 0000d3a7... ✅</code>
        </div>
      </div>
    </div>
  );
}

export default MiningAnimation;