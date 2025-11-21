#include "blockchain.h"
#include <iostream>

// 전방 선언
void runServer(Blockchain &blockchain);

int main()
{
    Blockchain chain;
    chain.setDifficulty(3);

    std::cout << "Starting ToyChain Blockchain Server...\n";
    runServer(chain);

    return 0;
}
