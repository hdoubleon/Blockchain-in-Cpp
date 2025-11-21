#ifndef BLOCK_H
#define BLOCK_H

#include "transaction.h"
#include <string>
#include <vector>
#include <ctime>

class Block
{
private:
    int index;
    long long timestamp;
    std::vector<Transaction> transactions;
    std::string previousHash;
    std::string hash;
    int nonce;
    int difficulty;

public:
    Block(int idx, const std::vector<Transaction> &txs, const std::string &prevHash);
    void setTimestamp(long long time) { timestamp = time; }
    void setHash(const std::string &newHash) { hash = newHash; }
    void mineBlock(int difficulty);
    std::string calculateHash() const;

    int getIndex() const { return index; }
    long long getTimestamp() const { return timestamp; }
    const std::vector<Transaction> &getTransactions() const { return transactions; }
    std::string getPreviousHash() const { return previousHash; }
    std::string getHash() const { return hash; }
    int getNonce() const { return nonce; }
    int getDifficulty() const { return difficulty; }
};

#endif
