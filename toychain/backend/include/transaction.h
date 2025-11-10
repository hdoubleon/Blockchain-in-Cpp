#pragma once

#include <string>

class Transaction {
public:
    Transaction() = default;
    Transaction(std::string sender, std::string recipient, double amount);

    const std::string& getSender() const noexcept;
    const std::string& getRecipient() const noexcept;
    double getAmount() const noexcept;

    std::string toString() const;

private:
    std::string sender_;
    std::string recipient_;
    double amount_{0.0};
};
