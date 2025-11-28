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

void Block::mineBlock(int diff)
{
    difficulty = diff;
    std::string target(diff, '0');

    while (hash.substr(0, diff) != target)
    {
        nonce++;
        hash = calculateHash();
    }

    std::cout << "Block mined: " << hash << std::endl;
}
