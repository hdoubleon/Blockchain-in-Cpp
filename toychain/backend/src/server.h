#ifndef SERVER_H
#define SERVER_H

#include "blockchain.h"
#include <string>

void runServer(Blockchain &blockchain, const std::string &statePath);

#endif
