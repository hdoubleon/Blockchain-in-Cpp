#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <sqlite3.h>
#include <string>
#include "../block.h"
#include "../utxo.h"
#include <vector>
#include <filesystem>

class Database
{
private:
    sqlite3 *db;
    bool opened = false;

public:
    Database(const std::string &filename);
    ~Database();

    bool isOpen() const { return opened; }

    bool exec(const std::string &sql);
    void createTables();

    // WRITE FUNCTIONS
    bool insertBlock(const Block &block, const std::vector<UTXOTransaction> &txs);
    bool upsertMempool(const std::vector<UTXOTransaction> &pending);
};

#endif
