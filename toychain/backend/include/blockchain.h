#pragma once

#include "block.h"

#include <string>
#include <vector>

class Blockchain {
public:
    Blockchain();

    void setDifficulty(std::size_t difficulty) noexcept;
    std::size_t getDifficulty() const noexcept;

    void addTransaction(Transaction transaction);
    void minePendingTransactions(const std::string& minerAddress);

    bool isChainValid() const;

    const std::vector<Block>& getChain() const noexcept;

private:
    std::vector<Block> chain_;
    std::vector<Transaction> pendingTransactions_;
    std::size_t difficulty_{2};
    double miningReward_{10.0};

    Block createGenesisBlock() const;
};
