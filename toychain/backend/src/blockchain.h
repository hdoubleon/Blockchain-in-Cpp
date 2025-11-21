#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include "transaction.h"
#include <vector>
#include <unordered_map>

class Blockchain
{
private:
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions;
    int difficulty;
    double miningReward;

    int blockTimeTarget;
    int difficultyAdjustmentInterval;

public:
    Blockchain();

    Block createGenesisBlock();
    Block getLatestBlock() const;
    void minePendingTransactions(const std::string &miningRewardAddress);
    void addTransaction(const Transaction &transaction);

    bool isChainValid() const;

    const std::vector<Block> &getChain() const { return chain; }
    int getDifficulty() const { return difficulty; }
    void setDifficulty(int diff) { difficulty = diff; }

    void setBlockTimeTarget(int seconds) { blockTimeTarget = seconds; }
    void setDifficultyAdjustmentInterval(int blocks) { difficultyAdjustmentInterval = blocks; }

    void adjustDifficulty();
    int calculateNewDifficulty() const;

    std::unordered_map<std::string, double> getBalances() const;
};

#endif