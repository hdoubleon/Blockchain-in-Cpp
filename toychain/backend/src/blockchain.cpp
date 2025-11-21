#include "blockchain.h"
#include <iostream>
#include <unordered_map>

Blockchain::Blockchain()
{
    chain.push_back(createGenesisBlock());
    difficulty = 2;
    miningReward = 10.0;
    blockTimeTarget = 10;
    difficultyAdjustmentInterval = 5;
}

Block Blockchain::createGenesisBlock()
{
    std::vector<Transaction> genesisTransactions;
    Block genesis(0, genesisTransactions, "0");
    genesis.setTimestamp(0);                  // <--- 이 줄 수정
    genesis.setHash(genesis.calculateHash()); // timestamp 바뀌었으니 hash도 다시 계산
    return genesis;
}

Block Blockchain::getLatestBlock() const
{
    return chain.back();
}

void Blockchain::addTransaction(const Transaction &transaction)
{
    pendingTransactions.push_back(transaction);
}

void Blockchain::minePendingTransactions(const std::string &minerAddress)
{
    // 난이도 자동 조정
    if (chain.size() % difficultyAdjustmentInterval == 0 && chain.size() > 0)
    {
        adjustDifficulty();
    }

    if (pendingTransactions.empty())
    {
        std::cout << "No pending transactions to mine.\n";
        return;
    }

    // 채굴 보상 트랜잭션 추가
    std::vector<Transaction> transactions = pendingTransactions;
    transactions.push_back(Transaction("MINING_REWARD", minerAddress, miningReward));

    Block block(chain.size(), transactions, getLatestBlock().getHash());
    block.mineBlock(difficulty);

    chain.push_back(block);
    pendingTransactions.clear();

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

    for (const auto &block : chain)
    {
        for (const auto &tx : block.getTransactions())
        {
            balances[tx.getRecipient()] += tx.getAmount();
            if (tx.getSender() != "MINING_REWARD")
            {
                balances[tx.getSender()] -= tx.getAmount();
            }
        }
    }

    return balances;
}
