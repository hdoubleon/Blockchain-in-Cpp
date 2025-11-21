#include "blockchain.h"
#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstring>

void handleRequest(int client_socket, Blockchain &blockchain)
{
    if (client_socket < 0)
        return; // 방어 코드

    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_socket, buffer, 4096);
    if (bytes_read <= 0)
    {
        close(client_socket);
        return;
    }
    std::string request(buffer, bytes_read);
    std::istringstream iss(request);
    std::string method, path;
    iss >> method >> path;

    std::string response_body;
    std::string content_type = "application/json";

    if (path == "/blockchain")
    {
        response_body = "{\"chain\":[";
        const auto &chain = blockchain.getChain();
        for (size_t i = 0; i < chain.size(); i++)
        {
            const auto &block = chain[i];
            response_body += "{";
            response_body += "\"index\":" + std::to_string(block.getIndex()) + ",";
            response_body += "\"timestamp\":" + std::to_string(block.getTimestamp()) + ",";
            response_body += "\"hash\":\"" + block.getHash() + "\",";
            response_body += "\"previousHash\":\"" + block.getPreviousHash() + "\",";
            response_body += "\"nonce\":" + std::to_string(block.getNonce()) + ",";
            response_body += "\"difficulty\":" + std::to_string(block.getDifficulty()) + ",";
            response_body += "\"transactions\":[";

            const auto &txs = block.getTransactions();
            for (size_t j = 0; j < txs.size(); j++)
            {
                response_body += "{";
                response_body += "\"sender\":\"" + txs[j].getSender() + "\",";
                response_body += "\"recipient\":\"" + txs[j].getRecipient() + "\",";
                response_body += "\"amount\":" + std::to_string(txs[j].getAmount());
                response_body += "}";
                if (j < txs.size() - 1)
                    response_body += ",";
            }
            response_body += "]}";
            if (i < chain.size() - 1)
                response_body += ",";
        }
        response_body += "],\"difficulty\":" + std::to_string(blockchain.getDifficulty()) + "}";
    }
    else if (path == "/balances")
    {
        auto balances = blockchain.getBalances();
        response_body = "{";
        bool first = true;
        for (const auto &[address, balance] : balances)
        {
            if (!first)
                response_body += ",";
            response_body += "\"" + address + "\":" + std::to_string(balance);
            first = false;
        }
        response_body += "}";
    }
    else if (path == "/transaction" && method == "POST")
    {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            std::string body = request.substr(body_start + 4);

            size_t sender_pos = body.find("\"sender\":\"") + 10;
            size_t sender_end = body.find("\"", sender_pos);
            std::string sender = body.substr(sender_pos, sender_end - sender_pos);

            size_t recipient_pos = body.find("\"recipient\":\"") + 13;
            size_t recipient_end = body.find("\"", recipient_pos);
            std::string recipient = body.substr(recipient_pos, recipient_end - recipient_pos);

            size_t amount_pos = body.find("\"amount\":") + 9;
            size_t amount_end = body.find_first_of(",}", amount_pos);
            double amount = std::stod(body.substr(amount_pos, amount_end - amount_pos));

            blockchain.addTransaction(Transaction(sender, recipient, amount));
            response_body = "{\"status\":\"success\"}";
        }
    }
    else if (path == "/mine" && method == "POST")
    {
        size_t body_start = request.find("\r\n\r\n");
        std::string miner = "default_miner";

        if (body_start != std::string::npos)
        {
            std::string body = request.substr(body_start + 4);
            size_t miner_pos = body.find("\"miner\":\"");
            if (miner_pos != std::string::npos)
            {
                miner_pos += 9;
                size_t miner_end = body.find("\"", miner_pos);
                miner = body.substr(miner_pos, miner_end - miner_pos);
            }
        }

        std::cout << "Mining started by: " << miner << "\n";
        blockchain.minePendingTransactions(miner);
        std::cout << "Mining completed!\n";

        response_body = "{\"status\":\"success\"}";
    }
    else
    {
        response_body = "{\"error\":\"Not found\"}";
    }

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response += "Access-Control-Allow-Headers: Content-Type\r\n";
    response += "\r\n";
    response += response_body;

    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
}

void runServer(Blockchain &blockchain)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed\n";
        return;
    }
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed\n";
        return;
    }

    std::cout << "Server listening on port 8080...\n";

    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            std::cerr << "Accept failed\n";
            continue;
        }
        handleRequest(client_socket, blockchain);
    }
}