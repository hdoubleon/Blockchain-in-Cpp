#include "utxo.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <chrono>

UTXOTransaction::UTXOTransaction(const std::vector<TxInput> &ins, const std::vector<TxOutput> &outs)
    : entropy(generateEntropy()), inputs(ins), outputs(outs)
{
    id = calculateHash();
}

std::string UTXOTransaction::calculateHash() const
{
    std::stringstream ss;

    for (const auto &input : inputs)
    {
        ss << input.txId << input.outputIndex << input.signature;
    }

    for (const auto &output : outputs)
    {
        ss << output.address << output.amount;
    }

    ss << entropy; // ensure coinbase tx ids differ per block

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

std::string UTXOTransaction::generateEntropy() const
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::to_string(now);
}

std::string UTXOTransaction::toString() const
{
    std::stringstream ss;
    ss << "TX[" << id.substr(0, 8) << "...] ";
    ss << "Inputs: " << inputs.size() << ", Outputs: " << outputs.size();
    return ss.str();
}

// UTXOSet implementation
void UTXOSet::addUTXO(const std::string &txId, int index, const TxOutput &output)
{
    utxos[makeKey(txId, index)] = output;
}

bool UTXOSet::removeUTXO(const std::string &txId, int index)
{
    return utxos.erase(makeKey(txId, index)) > 0;
}

bool UTXOSet::hasUTXO(const std::string &txId, int index) const
{
    return utxos.find(makeKey(txId, index)) != utxos.end();
}

TxOutput UTXOSet::getUTXO(const std::string &txId, int index) const
{
    return utxos.at(makeKey(txId, index));
}

double UTXOSet::getBalance(const std::string &address) const
{
    double balance = 0.0;
    for (const auto &[key, output] : utxos)
    {
        if (output.address == address)
        {
            balance += output.amount;
        }
    }
    return balance;
}

std::vector<std::pair<std::string, TxOutput>> UTXOSet::getUTXOsForAddress(const std::string &address) const
{
    std::vector<std::pair<std::string, TxOutput>> result;
    for (const auto &[key, output] : utxos)
    {
        if (output.address == address)
        {
            result.push_back({key, output});
        }
    }
    return result;
}

std::unordered_map<std::string, TxOutput> UTXOSet::getAllUTXOs() const
{
    return utxos;
}

std::string UTXOSet::makeKey(const std::string &txId, int index) const
{
    return txId + ":" + std::to_string(index);
}
