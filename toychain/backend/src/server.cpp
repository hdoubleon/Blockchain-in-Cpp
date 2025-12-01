#include "blockchain.h"
#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <random>
#include <chrono>
#include <functional>

struct MiningJob
{
    std::string id;
    bool done = false;
    bool error = false;
    std::string errMsg;
    std::string hash;
    int nonce = 0;
    int difficulty = 0;
    std::vector<std::string> attempts;
    std::mutex mtx;
};

std::unordered_map<std::string, std::shared_ptr<MiningJob>> jobs;
std::mutex jobsMutex;

std::string makeJobId()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::mt19937_64 gen(now);
    std::uniform_int_distribution<uint64_t> dist;
    std::stringstream ss;
    ss << std::hex << now << dist(gen);
    return ss.str();
}
#include <string>

void handleRequest(int client_socket, Blockchain &blockchain, const std::string &statePath)
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
                const auto &tx = txs[j];
                response_body += "{";
                response_body += "\"id\":\"" + tx.getId() + "\",";
                response_body += "\"inputs\":[";
                const auto &inputs = tx.getInputs();
                for (size_t k = 0; k < inputs.size(); ++k)
                {
                    const auto &in = inputs[k];
                    response_body += "{";
                    response_body += "\"txId\":\"" + in.txId + "\",";
                    response_body += "\"outputIndex\":" + std::to_string(in.outputIndex) + ",";
                    response_body += "\"signature\":\"" + in.signature + "\"";
                    response_body += "}";
                    if (k < inputs.size() - 1)
                        response_body += ",";
                }
                response_body += "],";

                response_body += "\"outputs\":[";
                const auto &outputs = tx.getOutputs();
                for (size_t k = 0; k < outputs.size(); ++k)
                {
                    const auto &out = outputs[k];
                    response_body += "{";
                    response_body += "\"address\":\"" + out.address + "\",";
                    response_body += "\"amount\":" + std::to_string(out.amount);
                    response_body += "}";
                    if (k < outputs.size() - 1)
                        response_body += ",";
                }
                response_body += "]}";

                if (j < txs.size() - 1)
                    response_body += ",";
            }
            response_body += "]}";
            if (i < chain.size() - 1)
                response_body += ",";
        }
        response_body += "],\"difficulty\":" + std::to_string(blockchain.getDifficulty()) + "}";
    }
    else if (path == "/difficulty")
    {
        if (method == "GET")
        {
            response_body = "{\"difficulty\":" + std::to_string(blockchain.getDifficulty()) + "}";
        }
        else if (method == "POST")
        {
            size_t body_start = request.find("\r\n\r\n");
            if (body_start != std::string::npos)
            {
                std::string body = request.substr(body_start + 4);
                size_t diff_pos = body.find("\"difficulty\":");
                if (diff_pos != std::string::npos)
                {
                    diff_pos += 13;
                    size_t diff_end = body.find_first_of(",}", diff_pos);
                    int newDiff = std::stoi(body.substr(diff_pos, diff_end - diff_pos));
                    if (newDiff < 1)
                        newDiff = 1;
                    blockchain.setDifficulty(newDiff);
                    response_body = "{\"status\":\"success\",\"difficulty\":" + std::to_string(newDiff) + "}";
                }
                else
                {
                    response_body = "{\"status\":\"error\",\"message\":\"difficulty not provided\"}";
                }
            }
        }
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
    else if (path == "/pending")
    {
        const auto &pending = blockchain.getPendingTransactions();
        response_body = "[";
        for (size_t j = 0; j < pending.size(); ++j)
        {
            const auto &tx = pending[j];
            response_body += "{";
            response_body += "\"id\":\"" + tx.getId() + "\",";
            response_body += "\"inputs\":[";
            const auto &inputs = tx.getInputs();
            for (size_t k = 0; k < inputs.size(); ++k)
            {
                const auto &in = inputs[k];
                response_body += "{";
                response_body += "\"txId\":\"" + in.txId + "\",";
                response_body += "\"outputIndex\":" + std::to_string(in.outputIndex) + ",";
                response_body += "\"signature\":\"" + in.signature + "\"";
                response_body += "}";
                if (k < inputs.size() - 1)
                    response_body += ",";
            }
            response_body += "],";

            response_body += "\"outputs\":[";
            const auto &outputs = tx.getOutputs();
            for (size_t k = 0; k < outputs.size(); ++k)
            {
                const auto &out = outputs[k];
                response_body += "{";
                response_body += "\"address\":\"" + out.address + "\",";
                response_body += "\"amount\":" + std::to_string(out.amount);
                response_body += "}";
                if (k < outputs.size() - 1)
                    response_body += ",";
            }
            response_body += "]}";
            if (j < pending.size() - 1)
                response_body += ",";
        }
        response_body += "]";
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

            std::string error;
            bool ok = blockchain.addTransaction(sender, recipient, amount, error);
            if (ok)
            {
                response_body = "{\"status\":\"success\"}";
            }
            else
            {
                response_body = "{\"status\":\"error\",\"message\":\"" + error + "\"}";
            }
        }
    }
    else if (path == "/mine" && method == "POST")
    {
        // legacy synchronous mining kept for compatibility
        std::string miner = "default_miner";
        size_t body_start = request.find("\r\n\r\n");
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

        std::vector<std::string> attempts;
        blockchain.minePendingTransactions(miner, [&](const std::string &h, int n) {
            attempts.push_back(std::to_string(n) + ":" + h);
            if (attempts.size() > 50)
                attempts.erase(attempts.begin());
        });
        blockchain.saveToFile(statePath);

        const Block &latest = blockchain.getLatestBlock();
        response_body = "{";
        response_body += "\"status\":\"success\",";
        response_body += "\"hash\":\"" + latest.getHash() + "\",";
        response_body += "\"nonce\":" + std::to_string(latest.getNonce()) + ",";
        response_body += "\"difficulty\":" + std::to_string(latest.getDifficulty()) + ",";
        response_body += "\"attempts\":[";
        for (size_t i = 0; i < attempts.size(); ++i)
        {
            response_body += "\"" + attempts[i] + "\"";
            if (i < attempts.size() - 1)
                response_body += ",";
        }
        response_body += "]";
        response_body += "}";
    }
    else if (path == "/mine/start" && method == "POST")
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

        auto job = std::make_shared<MiningJob>();
        job->id = makeJobId();
        job->difficulty = blockchain.getDifficulty();

        {
            std::lock_guard<std::mutex> lock(jobsMutex);
            jobs[job->id] = job;
        }

        std::thread([job, miner, statePath, &blockchain]() {
            try
            {
                blockchain.minePendingTransactions(miner, [job](const std::string &h, int n) {
                    std::lock_guard<std::mutex> lk(job->mtx);
                    job->attempts.push_back(std::to_string(n) + ":" + h);
                    if (job->attempts.size() > 100)
                        job->attempts.erase(job->attempts.begin());
                });
                blockchain.saveToFile(statePath);

                const Block &latest = blockchain.getLatestBlock();
                std::lock_guard<std::mutex> lk(job->mtx);
                job->done = true;
                job->hash = latest.getHash();
                job->nonce = latest.getNonce();
                job->difficulty = latest.getDifficulty();
            }
            catch (const std::exception &e)
            {
                std::lock_guard<std::mutex> lk(job->mtx);
                job->error = true;
                job->errMsg = e.what();
            }
        }).detach();

        response_body = "{\"status\":\"started\",\"jobId\":\"" + job->id + "\"}";
    }
    else if (path.rfind("/mine/status", 0) == 0 && method == "GET")
    {
        size_t q = path.find("id=");
        if (q == std::string::npos)
        {
            response_body = "{\"status\":\"error\",\"message\":\"job id missing\"}";
        }
        else
        {
            std::string jobId = path.substr(q + 3);
            std::shared_ptr<MiningJob> job;
            {
                std::lock_guard<std::mutex> lock(jobsMutex);
                auto it = jobs.find(jobId);
                if (it != jobs.end())
                    job = it->second;
            }

            if (!job)
            {
                response_body = "{\"status\":\"error\",\"message\":\"job not found\"}";
            }
            else
            {
                std::lock_guard<std::mutex> lk(job->mtx);
                response_body = "{";
                if (job->error)
                {
                    response_body += "\"status\":\"error\",\"message\":\"" + job->errMsg + "\"";
                }
                else if (job->done)
                {
                    response_body += "\"status\":\"done\",";
                    response_body += "\"hash\":\"" + job->hash + "\",";
                    response_body += "\"nonce\":" + std::to_string(job->nonce) + ",";
                    response_body += "\"difficulty\":" + std::to_string(job->difficulty) + ",";
                    response_body += "\"attempts\":[";
                    for (size_t i = 0; i < job->attempts.size(); ++i)
                    {
                        response_body += "\"" + job->attempts[i] + "\"";
                        if (i < job->attempts.size() - 1)
                            response_body += ",";
                    }
                    response_body += "]";
                }
                else
                {
                    response_body += "\"status\":\"running\",";
                    response_body += "\"attempts\":[";
                    for (size_t i = 0; i < job->attempts.size(); ++i)
                    {
                        response_body += "\"" + job->attempts[i] + "\"";
                        if (i < job->attempts.size() - 1)
                            response_body += ",";
                    }
                    response_body += "]";
                }
                response_body += "}";
            }
        }
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

void runServer(Blockchain &blockchain, const std::string &statePath)
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
        handleRequest(client_socket, blockchain, statePath);
    }
}
