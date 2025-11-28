# ToyChain

ToyChain is a minimal blockchain playground that consists of:

- **backend** – a C++17 implementation of a tiny blockchain with blocks, transactions, and simple proof-of-work style mining.
- **frontend** – a React interface that visualizes the chain and lets you submit transactions and trigger mining against the running node.

## Backend

```bash
cd backend
cmake -S . -B build
cmake --build build
./build/toychain_server
```

### REST API (UTXO)

- `GET /blockchain` → `{ chain: Block[], difficulty: number }`
- `GET /balances` → `{ [address]: number }` derived from current UTXO set
- `POST /transaction` body `{"sender":"alice","recipient":"bob","amount":1.5}` → enqueues a spend (validated against UTXOs)
- `POST /mine` body `{"miner":"addr"}` → mines pending txs plus coinbase paying `miner`

### Persistence

The node writes chain state to `../data/chain.dat` (relative to `backend/build`) after every successful mine. On startup it will attempt to load that file; if missing, a fresh chain with only the genesis block is created.

## Frontend

The frontend uses Vite. Install dependencies and start the dev server.

```bash
cd frontend
npm install
npm run dev
```

## Project Layout

```
toychain/
├── backend/
│   ├── include/           # C++ headers
│   ├── src/               # C++ sources
│   ├── CMakeLists.txt     # Build configuration
│   └── build/             # Out-of-source build directory (empty)
├── frontend/
│   ├── src/               # React source files
│   └── package.json       # Frontend dependencies & scripts
└── README.md
```
