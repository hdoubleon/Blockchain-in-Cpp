# ToyChain

ToyChain is a minimal blockchain playground that consists of:

- **backend** – a C++17 implementation of a tiny blockchain with blocks, transactions, and simple proof-of-work style mining.
- **frontend** – a React interface that visualizes the chain and lets you queue transactions before mining a new block.

## Backend

```bash
cd backend
cmake -S . -B build
cmake --build build
./build/toychain
```

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
