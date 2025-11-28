#pragma once

#include <string>
#include <vector>
#include <sqlite3.h>

class Block;
class UTXOTransaction;

class Database
{
public:
    explicit Database(const std::string &filename);
    ~Database();

    bool isOpen() const { return opened; }

    void createTables();
    bool insertBlock(const Block &block, const std::vector<UTXOTransaction> &txs);
    bool upsertMempool(const std::vector<UTXOTransaction> &pending);

private:
    sqlite3 *db = nullptr;
    bool opened = false;

    bool exec(const std::string &sql);
};
