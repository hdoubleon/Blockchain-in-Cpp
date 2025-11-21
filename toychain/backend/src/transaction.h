#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

class Transaction
{
private:
    std::string sender;
    std::string recipient;
    double amount;

public:
    Transaction(const std::string &from, const std::string &to, double amt);

    std::string getSender() const { return sender; }
    std::string getRecipient() const { return recipient; }
    double getAmount() const { return amount; }

    std::string toString() const;
};

#endif
