#include "blockchain.h"
#include "transaction.h"

#include <iostream>

int main() {
    Blockchain chain;
    chain.setDifficulty(3);

    chain.addTransaction(Transaction("Alice", "Bob", 5.0));
    chain.addTransaction(Transaction("Charlie", "Dave", 2.5));

    std::cout << "Mining pending transactions...\n";
    chain.minePendingTransactions("Miner1");

    for (const auto& block : chain.getChain()) {
        std::cout << "Block #" << block.getIndex() << "\n";
        std::cout << "  Timestamp: " << block.getTimestamp() << "\n";
        std::cout << "  Hash: " << block.getHash() << "\n";
        std::cout << "  Prev: " << block.getPreviousHash() << "\n";
        std::cout << "  Transactions:\n";
        for (const auto& tx : block.getTransactions()) {
            std::cout << "    - " << tx.toString() << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Blockchain valid? " << (chain.isChainValid() ? "Yes" : "No") << "\n";
    return 0;
}
