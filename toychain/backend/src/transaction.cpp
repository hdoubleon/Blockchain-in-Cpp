#include "transaction.h"

#include <sstream>
#include <utility>

Transaction::Transaction(std::string sender, std::string recipient, double amount)
    : sender_(std::move(sender)), recipient_(std::move(recipient)), amount_(amount) {}

const std::string& Transaction::getSender() const noexcept {
    return sender_;
}

const std::string& Transaction::getRecipient() const noexcept {
    return recipient_;
}

double Transaction::getAmount() const noexcept {
    return amount_;
}

std::string Transaction::toString() const {
    std::ostringstream oss;
    oss << sender_ << "->" << recipient_ << ":" << amount_;
    return oss.str();
}
