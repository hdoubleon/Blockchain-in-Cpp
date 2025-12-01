#ifndef BLOCK_H
#define BLOCK_H

#include "utxo.h"
#include <string>
#include <vector>
#include <ctime>
#include <functional>

class Block
{
private:
    int index;
    long long timestamp;
    std::vector<UTXOTransaction> transactions;
    std::string previousHash;
    std::string hash;
    int nonce;
    int difficulty;

public:
    Block(int idx, const std::vector<UTXOTransaction> &txs, const std::string &prevHash);
    void setTimestamp(long long time) { timestamp = time; }
    void setHash(const std::string &newHash) { hash = newHash; }
    void setNonce(int n) { nonce = n; }
    void setDifficulty(int diff) { difficulty = diff; }
    void mineBlock(int difficulty, std::function<void(const std::string &, int)> onSample = nullptr);
    std::string calculateHash() const;

    int getIndex() const { return index; }
    long long getTimestamp() const { return timestamp; }
    const std::vector<UTXOTransaction> &getTransactions() const { return transactions; }
    std::string getPreviousHash() const { return previousHash; }
    std::string getHash() const { return hash; }
    int getNonce() const { return nonce; }
    int getDifficulty() const { return difficulty; }

};

#endif
