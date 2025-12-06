#include "block.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <iostream>

Block::Block(int idx, const std::vector<UTXOTransaction> &txs, const std::string &prevHash)
    : index(idx), transactions(txs), previousHash(prevHash), nonce(0), difficulty(0)
{
    timestamp = std::time(nullptr);
    hash = calculateHash();
}
Block::Block(int idx,
             long long ts,
             const std::vector<UTXOTransaction> &txs,
             const std::string &prevHash,
             int nonceVal,
             int diffVal)
    : index(idx), timestamp(ts), transactions(txs), previousHash(prevHash), nonce(nonceVal), difficulty(diffVal)
{
    hash = calculateHash(); // 외부에서 받은 hash와 비교할 때 사용할 예정
}

std::string Block::calculateHash() const
{
    std::stringstream ss;
    ss << index << timestamp << previousHash << nonce;

    for (const auto &tx : transactions)
    {
        ss << tx.toString();
    }

    std::string data = ss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)data.c_str(), data.size(), hash);

    std::stringstream hashStr;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        hashStr << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return hashStr.str();
}

void Block::mineBlock(int diff, std::function<void(const std::string &, int)> onSample)
{
    difficulty = diff;
    std::string target(diff, '0');

    while (hash.substr(0, diff) != target)
    {
        nonce++;
        hash = calculateHash();

        if (onSample && nonce % 5000 == 0)
        {
            onSample(hash, nonce);
        }
    }

    std::cout << "Block mined: " << hash << std::endl;

    if (onSample)
        onSample(hash, nonce);
}
