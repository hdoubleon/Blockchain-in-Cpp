#include "transaction.h"
#include <sstream>

Transaction::Transaction(const std::string &from, const std::string &to, double amt)
    : sender(from), recipient(to), amount(amt) {}

std::string Transaction::toString() const
{
    std::stringstream ss;
    ss << sender << " -> " << recipient << ": " << amount;
    return ss.str();
}
