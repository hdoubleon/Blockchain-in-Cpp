#include "blockchain.h"
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <fstream>

Blockchain::Blockchain() : database(nullptr)
{
    chain.push_back(createGenesisBlock());
    difficulty = 2;
    miningReward = 10.0;
    blockTimeTarget = 10;
    difficultyAdjustmentInterval = 5;
}

Block Blockchain::createGenesisBlock()
{
    std::vector<UTXOTransaction> genesisTransactions;
    Block genesis(0, genesisTransactions, "0");
    genesis.setTimestamp(0);                  // timestamp 고정
    genesis.setHash(genesis.calculateHash()); // timestamp 바뀌었으니 hash도 다시 계산
    return genesis;
}

Block Blockchain::getLatestBlock() const
{
    return chain.back();
}

bool Blockchain::isUTXOInPending(const std::string &txId, int index) const
{
    for (const auto &tx : pendingTransactions)
    {
        for (const auto &input : tx.getInputs())
        {
            if (input.txId == txId && input.outputIndex == index)
            {
                return true;
            }
        }
    }
    return false;
}

bool Blockchain::addTransaction(const std::string &from, const std::string &to, double amount, std::string &error)
{
    if (amount <= 0)
    {
        error = "Amount must be positive.";
        return false;
    }

    auto available = utxoSet.getUTXOsForAddress(from);
    std::vector<TxInput> inputs;
    double collected = 0.0;

    for (const auto &[key, output] : available)
    {
        auto delimiter = key.find(':');
        if (delimiter == std::string::npos)
        {
            continue;
        }
        std::string txId = key.substr(0, delimiter);
        int outIndex = std::stoi(key.substr(delimiter + 1));

        if (isUTXOInPending(txId, outIndex))
        {
            continue; // 이미 사용 중인 UTXO는 건너뛴다
        }

        inputs.emplace_back(txId, outIndex, from);
        collected += output.amount;

        if (collected >= amount)
        {
            break;
        }
    }

    if (collected < amount)
    {
        std::ostringstream oss;
        oss << "Insufficient funds. Need " << amount << ", have " << collected << ".";
        error = oss.str();
        return false;
    }

    std::vector<TxOutput> outputs;
    outputs.emplace_back(amount, to);
    double change = collected - amount;
    if (change > 0)
    {
        outputs.emplace_back(change, from);
    }

    pendingTransactions.emplace_back(inputs, outputs);
    if (database)
    {
        database->upsertMempool(pendingTransactions);
    }
    return true;
}

void Blockchain::applyTransactionToUTXOSet(const UTXOTransaction &tx)
{
    // Spend inputs
    for (const auto &input : tx.getInputs())
    {
        utxoSet.removeUTXO(input.txId, input.outputIndex);
    }

    // Add outputs
    const auto &outputs = tx.getOutputs();
    for (size_t i = 0; i < outputs.size(); ++i)
    {
        utxoSet.addUTXO(tx.getId(), static_cast<int>(i), outputs[i]);
    }
}

void Blockchain::minePendingTransactions(const std::string &minerAddress, std::function<void(const std::string &, int)> onSample)
{
    // 난이도 자동 조정
    if (chain.size() % difficultyAdjustmentInterval == 0 && chain.size() > 0)
    {
        adjustDifficulty();
    }

    // 채굴 보상 트랜잭션 (입력 없음, 보상 출력만)
    std::vector<TxInput> coinbaseInputs;
    std::vector<TxOutput> coinbaseOutputs = {TxOutput(miningReward, minerAddress)};
    UTXOTransaction coinbaseTx(coinbaseInputs, coinbaseOutputs);

    std::vector<UTXOTransaction> transactions;
    transactions.push_back(coinbaseTx);
    transactions.insert(transactions.end(), pendingTransactions.begin(), pendingTransactions.end());

    Block block(chain.size(), transactions, getLatestBlock().getHash());
    block.mineBlock(difficulty, onSample);

    // 블록 확정 후 UTXO 반영
    for (const auto &tx : transactions)
    {
        applyTransactionToUTXOSet(tx);
    }

    chain.push_back(block);
    pendingTransactions.clear();

    if (database)
    {
        database->insertBlock(block, transactions);
        database->upsertMempool(pendingTransactions);
    }

    std::cout << "Block successfully mined!\n";
}

bool Blockchain::isChainValid() const
{
    for (std::size_t i = 1; i < chain.size(); ++i)
    {
        const Block &current = chain[i];
        const Block &previous = chain[i - 1];

        if (current.getHash() != current.calculateHash())
        {
            return false;
        }

        if (current.getPreviousHash() != previous.getHash())
        {
            return false;
        }
    }
    return true;
}

void Blockchain::adjustDifficulty()
{
    int newDifficulty = calculateNewDifficulty();

    std::cout << "Difficulty adjustment: " << difficulty << " -> " << newDifficulty << "\n";
    difficulty = newDifficulty;
}

int Blockchain::calculateNewDifficulty() const
{
    if (chain.size() < static_cast<size_t>(difficultyAdjustmentInterval))
    {
        return difficulty;
    }

    int startIndex = chain.size() - difficultyAdjustmentInterval;
    long long startTime = chain[startIndex].getTimestamp();
    long long endTime = chain.back().getTimestamp();

    long long actualTime = endTime - startTime;
    long long expectedTime = blockTimeTarget * difficultyAdjustmentInterval;

    if (actualTime < expectedTime / 2)
    {
        return difficulty + 1;
    }
    else if (actualTime > expectedTime * 2)
    {
        return std::max(1, difficulty - 1);
    }

    return difficulty;
}

std::unordered_map<std::string, double> Blockchain::getBalances() const
{
    std::unordered_map<std::string, double> balances;
    auto all = utxoSet.getAllUTXOs();
    for (const auto &[key, output] : all)
    {
        balances[output.address] += output.amount;
    }
    return balances;
}

std::vector<std::tuple<std::string, int, TxOutput>> Blockchain::getUTXOs() const
{
    std::vector<std::tuple<std::string, int, TxOutput>> list;
    auto all = utxoSet.getAllUTXOs();
    for (const auto &[key, output] : all)
    {
        auto pos = key.find(':');
        if (pos == std::string::npos)
            continue;
        std::string txId = key.substr(0, pos);
        int outIdx = std::stoi(key.substr(pos + 1));
        list.emplace_back(txId, outIdx, output);
    }
    return list;
}

bool Blockchain::saveToFile(const std::string &path) const
{
    std::ofstream out(path);
    if (!out.is_open())
    {
        std::cerr << "Failed to open " << path << " for writing\n";
        return false;
    }

    out << "DIFFICULTY " << difficulty << "\n";
    out << "BLOCKS " << chain.size() << "\n";

    for (const auto &block : chain)
    {
        out << "BLOCK " << block.getIndex() << " " << block.getTimestamp() << " " << block.getNonce() << " " << block.getDifficulty() << "\n";
        out << "PREV " << block.getPreviousHash() << "\n";
        out << "HASH " << block.getHash() << "\n";

        const auto &txs = block.getTransactions();
        out << "TXCOUNT " << txs.size() << "\n";
        for (const auto &tx : txs)
        {
            out << "TX " << tx.getId() << " " << tx.getInputs().size() << " " << tx.getOutputs().size() << "\n";
            for (const auto &in : tx.getInputs())
            {
                out << "IN " << in.txId << " " << in.outputIndex << " " << in.signature << "\n";
            }
            for (const auto &outTx : tx.getOutputs())
            {
                out << "OUT " << outTx.amount << " " << outTx.address << "\n";
            }
        }
    }

    return true;
}

bool Blockchain::loadFromFile(const std::string &path)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }

    std::vector<Block> loadedChain;
    int loadedDifficulty = difficulty;
    std::string line;
    int declaredBlocks = 0;

    while (std::getline(in, line))
    {
        if (line.empty())
        {
            continue;
        }
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "DIFFICULTY")
        {
            iss >> loadedDifficulty;
        }
        else if (tag == "BLOCKS")
        {
            iss >> declaredBlocks;
        }
        else if (tag == "BLOCK")
        {
            int idx;
            long long ts;
            int nonce;
            int diff;
            iss >> idx >> ts >> nonce >> diff;

            std::string prevHashLine;
            std::string hashLine;
            std::string txCountLine;

            if (!std::getline(in, prevHashLine) || !std::getline(in, hashLine) || !std::getline(in, txCountLine))
            {
                std::cerr << "Corrupted state file while reading block headers.\n";
                return false;
            }

            std::istringstream prevIss(prevHashLine);
            std::istringstream hashIss(hashLine);
            std::istringstream txCountIss(txCountLine);
            std::string prevTag, hashTag, txCountTag;
            std::string prevHash;
            std::string hash;
            size_t txCount = 0;
            prevIss >> prevTag >> prevHash;
            hashIss >> hashTag >> hash;
            txCountIss >> txCountTag >> txCount;

            std::vector<UTXOTransaction> txs;
            for (size_t t = 0; t < txCount; ++t)
            {
                std::string txLine;
                if (!std::getline(in, txLine))
                {
                    std::cerr << "Corrupted state file while reading transaction header.\n";
                    return false;
                }
                std::istringstream txIss(txLine);
                std::string txTag, txId;
                size_t inCount = 0, outCount = 0;
                txIss >> txTag >> txId >> inCount >> outCount;

                std::vector<TxInput> inputs;
                for (size_t i = 0; i < inCount; ++i)
                {
                    std::string inLine;
                    std::getline(in, inLine);
                    std::istringstream inIss(inLine);
                    std::string inTag, inTxId, signature;
                    int outIndex;
                    inIss >> inTag >> inTxId >> outIndex >> signature;
                    inputs.emplace_back(inTxId, outIndex, signature);
                }

                std::vector<TxOutput> outputs;
                for (size_t o = 0; o < outCount; ++o)
                {
                    std::string outLine;
                    std::getline(in, outLine);
                    std::istringstream outIss(outLine);
                    std::string outTag, address;
                    double amount;
                    outIss >> outTag >> amount >> address;
                    outputs.emplace_back(amount, address);
                }

                UTXOTransaction tx(inputs, outputs);
                txs.push_back(tx);
            }

            Block block(idx, txs, prevHash);
            block.setTimestamp(ts);
            block.setNonce(nonce);
            block.setDifficulty(diff);
            block.setHash(hash);
            loadedChain.push_back(block);
        }
    }

    if (declaredBlocks != 0 && declaredBlocks != static_cast<int>(loadedChain.size()))
    {
        std::cerr << "State file block count mismatch.\n";
        return false;
    }

    chain = loadedChain;
    pendingTransactions.clear();
    utxoSet = UTXOSet();
    for (const auto &block : chain)
    {
        for (const auto &tx : block.getTransactions())
        {
            applyTransactionToUTXOSet(tx);
        }
    }
    difficulty = loadedDifficulty;
    return true;
}
