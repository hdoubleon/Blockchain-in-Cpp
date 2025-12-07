#include "blockchain.h"
#include <iostream>
#include "db/Database.hpp"

// 전방 선언
void runServer(Blockchain &blockchain, const std::string &statePath);

int main()
{
    Blockchain chain;
    chain.setDifficulty(3);

    const std::string dataDir = "../data";
    const std::string dbPath = dataDir + "/chain.db";

    Database db(dbPath);
    if (db.isOpen())
    {
        chain.attachDatabase(&db);
    }

    const std::string statePath = "../data/chain.dat";
    if (chain.loadFromFile(statePath))
    {
        std::cout << "Loaded blockchain state from " << statePath << "\n";
    }
    else
    {
        std::cout << "Starting new chain (no persisted state found).\n";
    }

    std::cout << "Starting ToyChain Blockchain Server...\n";
    runServer(chain, statePath);

    return 0;
}
