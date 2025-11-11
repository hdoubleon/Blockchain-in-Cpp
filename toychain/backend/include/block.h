#pragma once

#include "transaction.h"

#include <chrono>
#include <string>
#include <vector>

class Block {
public:
    Block(std::size_t index,
          std::vector<Transaction> transactions,
          std::string previousHash);

    const std::string& getHash() const noexcept;
    const std::string& getPreviousHash() const noexcept;
    std::size_t getIndex() const noexcept;
    const std::vector<Transaction>& getTransactions() const noexcept;
    std::string getTimestamp() const noexcept;

    void mine(std::size_t difficulty);
    bool hasValidHash() const;

private:
    std::size_t index_{0};
    std::string timestamp_;
    std::vector<Transaction> transactions_;
    std::string previousHash_;
    std::string hash_;
    std::size_t nonce_{0};

    std::string calculateHash() const;
    static std::string getCurrentTimestamp();
};
