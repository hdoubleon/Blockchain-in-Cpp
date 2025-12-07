#include "Database.hpp"
#include "../block.h"
#include "../utxo.h"
#include <iostream>
#include <sstream>
#include <filesystem>

Database::Database(const std::string &filename)
{
    try
    {
        auto dir = std::filesystem::path(filename).parent_path();
        if (!dir.empty())
        {
            std::filesystem::create_directories(dir);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Cannot create database directory: " << e.what() << std::endl;
    }

    if (sqlite3_open(filename.c_str(), &db))
    {
        std::cerr << "❌ Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        opened = false;
    }
    else
    {
        opened = true;
        std::cout << "✅ Database connected: " << filename << std::endl;
        createTables();
    }
}

Database::~Database()
{
    if (db)
    {
        sqlite3_close(db);
    }
}

bool Database::exec(const std::string &sql)
{
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "❌ SQL error: " << (errMsg ? errMsg : "") << std::endl;
        if (errMsg)
            sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void Database::createTables()
{
    // based on chain.sql, simplified for UTXO usage
    const char *sql = R"(
        PRAGMA foreign_keys = ON;
        CREATE TABLE IF NOT EXISTS Block(
            block_id TEXT PRIMARY KEY,
            height INTEGER,
            timestamp INTEGER,
            prev_hash TEXT,
            difficulty INTEGER,
            nonce INTEGER
        );
        CREATE TABLE IF NOT EXISTS Tx(
            tx_id TEXT PRIMARY KEY,
            block_id TEXT,
            FOREIGN KEY(block_id) REFERENCES Block(block_id) ON DELETE CASCADE
        );
        CREATE TABLE IF NOT EXISTS TxOutput(
            tx_id TEXT,
            output_index INTEGER,
            address TEXT,
            value REAL,
            PRIMARY KEY (tx_id, output_index),
            FOREIGN KEY (tx_id) REFERENCES Tx(tx_id) ON DELETE CASCADE
        );
        CREATE TABLE IF NOT EXISTS TxInput(
            tx_id TEXT,
            input_index INTEGER,
            referenced_tx_id TEXT,
            referenced_output_index INTEGER,
            signature TEXT,
            PRIMARY KEY (tx_id, input_index),
            FOREIGN KEY (tx_id) REFERENCES Tx(tx_id) ON DELETE CASCADE
        );
        CREATE TABLE IF NOT EXISTS Mempool(
            tx_id TEXT PRIMARY KEY,
            raw_data TEXT
        );
    )";

    exec(sql);
}

bool Database::insertBlock(const Block &block, const std::vector<UTXOTransaction> &txs)
{
    if (!opened)
        return false;

    exec("BEGIN TRANSACTION;");

    std::stringstream bsql;
    bsql << "INSERT OR REPLACE INTO Block(block_id,height,timestamp,prev_hash,difficulty,nonce) VALUES("
         << "'" << block.getHash() << "',"
         << block.getIndex() << ","
         << block.getTimestamp() << ","
         << "'" << block.getPreviousHash() << "',"
         << block.getDifficulty() << ","
         << block.getNonce() << ");";
    if (!exec(bsql.str()))
    {
        exec("ROLLBACK;");
        return false;
    }

    for (const auto &tx : txs)
    {
        std::stringstream txsql;
        txsql << "INSERT OR REPLACE INTO Tx(tx_id, block_id) VALUES('"
              << tx.getId() << "','" << block.getHash() << "');";
        if (!exec(txsql.str()))
        {
            exec("ROLLBACK;");
            return false;
        }

        int inputIdx = 0;
        for (const auto &in : tx.getInputs())
        {
            std::stringstream insql;
            insql << "INSERT OR REPLACE INTO TxInput(tx_id,input_index,referenced_tx_id,referenced_output_index,signature) VALUES('"
                  << tx.getId() << "',"
                  << inputIdx << ",'"
                  << in.txId << "',"
                  << in.outputIndex << ",'"
                  << in.signature << "');";
            if (!exec(insql.str()))
            {
                exec("ROLLBACK;");
                return false;
            }
            inputIdx++;
        }

        for (size_t outIdx = 0; outIdx < tx.getOutputs().size(); ++outIdx)
        {
            const auto &out = tx.getOutputs()[outIdx];
            std::stringstream outsql;
            outsql << "INSERT OR REPLACE INTO TxOutput(tx_id,output_index,address,value) VALUES('"
                   << tx.getId() << "',"
                   << outIdx << ",'"
                   << out.address << "',"
                   << out.amount << ");";
            if (!exec(outsql.str()))
            {
                exec("ROLLBACK;");
                return false;
            }
        }
    }

    exec("COMMIT;");
    return true;
}

bool Database::upsertMempool(const std::vector<UTXOTransaction> &pending)
{
    if (!opened)
        return false;

    exec("BEGIN TRANSACTION;");
    exec("DELETE FROM Mempool;"); // simple truncate + insert snapshot

    for (const auto &tx : pending)
    {
        std::stringstream raw;
        raw << "inputs:" << tx.getInputs().size() << ",outputs:" << tx.getOutputs().size();
        std::stringstream sql;
        sql << "INSERT OR REPLACE INTO Mempool(tx_id,raw_data) VALUES('"
            << tx.getId() << "','"
            << raw.str() << "');";
        if (!exec(sql.str()))
        {
            exec("ROLLBACK;");
            return false;
        }
    }

    exec("COMMIT;");
    return true;
}
