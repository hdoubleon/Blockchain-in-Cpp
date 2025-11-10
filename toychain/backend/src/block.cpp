#include "block.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace
{
    std::string hashToHex(std::size_t value)
    {
        std::ostringstream oss;
        oss << std::hex << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << value;
        return oss.str();
    }
} // namespace

Block::Block(std::size_t index, std::vector<Transaction> transactions, std::string previousHash)
    : index_(index),
      timestamp_(getCurrentTimestamp()),
      transactions_(std::move(transactions)),
      previousHash_(std::move(previousHash)),
      hash_(calculateHash()) {}

const std::string &Block::getHash() const noexcept
{
    return hash_;
}

const std::string &Block::getPreviousHash() const noexcept
{
    return previousHash_;
}

std::size_t Block::getIndex() const noexcept
{
    return index_;
}

const std::vector<Transaction> &Block::getTransactions() const noexcept
{
    return transactions_;
}

std::string Block::getTimestamp() const noexcept
{
    return timestamp_;
}

void Block::mine(std::size_t difficulty)
{
    std::string prefix(difficulty, '0');
    while (hash_.substr(0, difficulty) != prefix)
    {
        ++nonce_;
        hash_ = calculateHash();
    }
}

std::string Block::calculateHash() const
{
    std::ostringstream oss;
    oss << index_ << timestamp_ << previousHash_ << nonce_;
    for (const auto &tx : transactions_)
    {
        oss << tx.toString();
    }
    return hashToHex(std::hash<std::string>{}(oss.str()));
}

bool Block::hasValidHash() const
{
    return calculateHash() == hash_;
}

std::string Block::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
