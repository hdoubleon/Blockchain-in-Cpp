#include "blockchain.h"

#include <iostream>

Blockchain::Blockchain() {
    chain_.push_back(createGenesisBlock());
}

void Blockchain::setDifficulty(std::size_t difficulty) noexcept {
    difficulty_ = difficulty > 0 ? difficulty : 1;
}

std::size_t Blockchain::getDifficulty() const noexcept {
    return difficulty_;
}

void Blockchain::addTransaction(Transaction transaction) {
    pendingTransactions_.push_back(std::move(transaction));
}

void Blockchain::minePendingTransactions(const std::string& minerAddress) {
    if (pendingTransactions_.empty()) {
        std::cout << "No transactions to mine.\n";
        return;
    }

    std::vector<Transaction> transactions = pendingTransactions_;
    transactions.emplace_back("network", minerAddress, miningReward_);

    Block block(chain_.size(), std::move(transactions), chain_.back().getHash());
    block.mine(difficulty_);
    chain_.push_back(std::move(block));

    pendingTransactions_.clear();
    pendingTransactions_.emplace_back("network", minerAddress, miningReward_);
}

bool Blockchain::isChainValid() const {
    for (std::size_t i = 1; i < chain_.size(); ++i) {
        const Block& current = chain_[i];
        const Block& previous = chain_[i - 1];

        if (current.getPreviousHash() != previous.getHash()) {
            return false;
        }
        if (!current.hasValidHash()) {
            return false;
        }
        if (current.getHash().substr(0, difficulty_) != std::string(difficulty_, '0')) {
            return false;
        }
    }
    return true;
}

const std::vector<Block>& Blockchain::getChain() const noexcept {
    return chain_;
}

Block Blockchain::createGenesisBlock() const {
    std::vector<Transaction> genesisTransactions{
        Transaction("network", "genesis", 0.0),
    };
    return Block(0, std::move(genesisTransactions), "0");
}
