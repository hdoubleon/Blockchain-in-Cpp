#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include "utxo.h"
#include "db/Database.hpp"
#include <vector>
#include <unordered_map>

class Blockchain
{
private:
    std::vector<Block> chain;
    std::vector<UTXOTransaction> pendingTransactions;
    UTXOSet utxoSet;
    Database *database;
    int difficulty;
    double miningReward;

    int blockTimeTarget;
    int difficultyAdjustmentInterval;

public:
    Blockchain();
    void attachDatabase(Database *db) { database = db; }

    Block createGenesisBlock();
    Block getLatestBlock() const;
    void minePendingTransactions(const std::string &miningRewardAddress, std::function<void(const std::string &, int)> onSample = nullptr);
    bool addTransaction(const std::string &from, const std::string &to, double amount, std::string &error);

    bool isChainValid() const;

    const std::vector<Block> &getChain() const { return chain; }
    int getDifficulty() const { return difficulty; }
    void setDifficulty(int diff) { difficulty = diff; }

    void setBlockTimeTarget(int seconds) { blockTimeTarget = seconds; }
    void setDifficultyAdjustmentInterval(int blocks) { difficultyAdjustmentInterval = blocks; }

    void adjustDifficulty();
    int calculateNewDifficulty() const;

    std::unordered_map<std::string, double> getBalances() const;
    const std::vector<UTXOTransaction> &getPendingTransactions() const { return pendingTransactions; }

    bool saveToFile(const std::string &path) const;
    bool loadFromFile(const std::string &path);

private:
    bool isUTXOInPending(const std::string &txId, int index) const;
    void applyTransactionToUTXOSet(const UTXOTransaction &tx);
};

#endif
