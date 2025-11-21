#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <vector>
#include <unordered_map>

struct TxInput
{
    std::string txId;      // 참조하는 이전 트랜잭션 ID
    int outputIndex;       // 해당 트랜잭션의 몇 번째 output인지
    std::string signature; // 서명 (간단히 address로 대체)

    TxInput(const std::string &id, int idx, const std::string &sig)
        : txId(id), outputIndex(idx), signature(sig) {}
};

struct TxOutput
{
    double amount;
    std::string address; // 수신자 주소

    TxOutput(double amt, const std::string &addr)
        : amount(amt), address(addr) {}
};

class UTXOTransaction
{
private:
    std::string id;
    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;

public:
    UTXOTransaction(const std::vector<TxInput> &ins, const std::vector<TxOutput> &outs);

    std::string getId() const { return id; }
    const std::vector<TxInput> &getInputs() const { return inputs; }
    const std::vector<TxOutput> &getOutputs() const { return outputs; }

    std::string calculateHash() const;
    std::string toString() const;
};

class UTXOSet
{
private:
    // key: txId:outputIndex, value: TxOutput
    std::unordered_map<std::string, TxOutput> utxos;

public:
    void addUTXO(const std::string &txId, int index, const TxOutput &output);
    bool removeUTXO(const std::string &txId, int index);
    bool hasUTXO(const std::string &txId, int index) const;
    TxOutput getUTXO(const std::string &txId, int index) const;

    double getBalance(const std::string &address) const;
    std::vector<std::pair<std::string, TxOutput>> getUTXOsForAddress(const std::string &address) const;

private:
    std::string makeKey(const std::string &txId, int index) const;
};

#endif
