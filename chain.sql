CREATE TABLE Block (
    block_id VARCHAR(64) PRIMARY KEY,
    height INT,
    timestamp DATETIME,
    prev_hash VARCHAR(64),
    merkle_root VARCHAR(64),
    difficulty INT,
    nonce INT
);

CREATE TABLE Transaction (
    tx_id VARCHAR(64) PRIMARY KEY,
    block_id VARCHAR(64),
    timestamp DATETIME,
    FOREIGN KEY (block_id) REFERENCES Block(block_id)

);

CREATE TABLE TxOutput(
    tx_id VARCHAR(64),
    output_index INT,
    address VARCHAR(64),
    value INT,
    is_spent BOOLEAN,
    PRIMARY KEY (tx_id, output_index),
    FOREIGN KEY (tx_id) REFERENCES Transaction(tx_id)
);

CREATE TABLE TxInput (
    input_id INT PRIMARY KEY,
    tx_id VARCHAR(64),
    referenced_tx_id VARCHAR(64),
    referenced_output_index INT,
    unlocking_script TEXT,
    FOREIGN KEY (tx_id) REFERENCES Transaction(tx_id),
    FOREIGN KEY (referenced_tx_id, referenced_output_index)
);

CREATE TABLE Node (
    node_id INT PRIMARY KEY,
    ip_address VARCAHR(50),
    PORT INT,
    status VARCHAR(20),
    last_seen DATETIME
);

CREATE TABLE MiningLog(
    log_id INT PRIMARY KEY,
    node_id INT,
    block_id VARCHAR(64),
    difficulty INT,
    time_taken_ms INT,
    created_at DATETIME,
    FOREIGN KEY (node_id) REFERENCES Node(node_id),
    FOREIGN KEY (block_id) REFERENCES Block(block_id)
);

CREATE TABLE AddressBalance(
    address VARCHAR(64) PRIMARY KEY,
    balance INT,
    last_updated_block INT
);

CREATE TABLE Mempool (
    tx_id Varchar(64) PRIMARY KEY,
    timestamp DATETIME,
    raw_date TEXT
);