import React from "react";
import "./MiningAnimation.css";

function MiningAnimation({ attempts = [], difficulty = 1 }) {
  return (
    <div className="mining-overlay">
      <div className="mining-box">
        <div className="block-icon rotating">📦</div>
        <h2>⛏️ Mining Block...</h2>
        <p>Finding hash with {difficulty} leading zero(s)</p>
        <div className="scroll-attempts">
          {attempts.map((a, idx) => (
            <div key={idx} className={`attempt ${a.success ? "success" : ""}`}>
              <code>{a.hash}</code>
              <span>{a.success ? "✅" : "❌"}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

export default MiningAnimation;
