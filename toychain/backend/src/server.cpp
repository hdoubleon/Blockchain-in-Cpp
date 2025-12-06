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
#include <tuple>
#include <stdexcept>
#include <cstdlib>
#include <netdb.h>
#include <string>

static std::vector<std::string> peers;

struct ParsedUrl
{
    std::string host;
    int port;
    std::string path;
};

static ParsedUrl parseUrl(const std::string &url)
{
    ParsedUrl u;
    std::string trimmed = url;
    if (trimmed.rfind("http://", 0) == 0)
        trimmed = trimmed.substr(7);
    auto slash = trimmed.find('/');
    std::string hostport = slash == std::string::npos ? trimmed : trimmed.substr(0, slash);
    u.path = slash == std::string::npos ? "/" : trimmed.substr(slash);
    auto colon = hostport.find(':');
    if (colon == std::string::npos)
    {
        u.host = hostport;
        u.port = 80;
    }
    else
    {
        u.host = hostport.substr(0, colon);
        u.port = std::stoi(hostport.substr(colon + 1));
    }
    return u;
}

static void postToPeer(const std::string &peerUrl, const std::string &path, const std::string &body)
{
    ParsedUrl p = parseUrl(peerUrl);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return;

    struct hostent *server = gethostbyname(p.host.c_str());
    if (!server)
    {
        close(sock);
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(p.port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sock);
        return;
    }

    std::stringstream req;
    req << "POST " << path << " HTTP/1.1\r\n";
    req << "Host: " << p.host << "\r\n";
    req << "Content-Type: application/json\r\n";
    req << "Content-Length: " << body.size() << "\r\n";
    req << "Connection: close\r\n\r\n";
    req << body;

    auto reqStr = req.str();
    send(sock, reqStr.c_str(), reqStr.size(), 0);
    close(sock);
}

static void broadcastJson(const std::string &path, const std::string &body)
{
    for (const auto &peer : peers)
    {
        postToPeer(peer, path, body);
    }
}

static void initPeersFromEnv()
{
    const char *env = std::getenv("PEERS");
    if (!env)
        return;
    std::string s(env);
    size_t start = 0;
    while (start < s.size())
    {
        auto comma = s.find(',', start);
        std::string one = s.substr(start, comma == std::string::npos ? s.size() - start : comma - start);
        if (!one.empty())
            peers.push_back(one);
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
}

// 매우 단순한 파서: 문자열을 찾아서 잘라내는 방식
static std::string extract(const std::string &body, const std::string &key)
{
    auto pos = body.find(key);
    if (pos == std::string::npos)
        return "";
    pos += key.size();
    auto end = body.find_first_of(",}", pos);
    return body.substr(pos, end - pos);
}

static std::string extractQuoted(const std::string &body, const std::string &key)
{
    auto pos = body.find(key);
    if (pos == std::string::npos)
        return "";
    pos += key.size();
    auto end = body.find("\"", pos);
    return body.substr(pos, end - pos);
}

static UTXOTransaction parseTxJson(const std::string &body)
{
    // body 예시: {"tx_id":"...","inputs":[...],"outputs":[...]}
    std::string txId = extractQuoted(body, "\"tx_id\":\"");
    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;

    // inputs 파싱 (수동): "inputs":[{...},{...}]
    size_t inArr = body.find("\"inputs\"");
    if (inArr != std::string::npos)
    {
        size_t inStart = body.find("[", inArr);
        size_t inEnd = body.find("]", inStart);
        std::string inputsChunk = body.substr(inStart + 1, inEnd - inStart - 1);
        size_t cursor = 0;
        while (true)
        {
            auto txPos = inputsChunk.find("\"txId\":\"", cursor);
            if (txPos == std::string::npos)
                break;
            txPos += 8;
            auto txEnd = inputsChunk.find("\"", txPos);
            std::string refTx = inputsChunk.substr(txPos, txEnd - txPos);

            auto outPos = inputsChunk.find("\"outputIndex\":", txEnd);
            outPos += 14;
            auto outEnd = inputsChunk.find_first_of(",}", outPos);
            int outIdx = std::stoi(inputsChunk.substr(outPos, outEnd - outPos));

            auto sigPos = inputsChunk.find("\"signature\":\"", outEnd);
            sigPos += 13;
            auto sigEnd = inputsChunk.find("\"", sigPos);
            std::string sig = inputsChunk.substr(sigPos, sigEnd - sigPos);

            inputs.emplace_back(refTx, outIdx, sig);
            cursor = sigEnd;
        }
    }

    // outputs 파싱: "outputs":[{...},{...}]
    size_t outArr = body.find("\"outputs\"");
    if (outArr != std::string::npos)
    {
        size_t oStart = body.find("[", outArr);
        size_t oEnd = body.find("]", oStart);
        std::string outsChunk = body.substr(oStart + 1, oEnd - oStart - 1);
        size_t cursor = 0;
        while (true)
        {
            auto addrPos = outsChunk.find("\"address\":\"", cursor);
            if (addrPos == std::string::npos)
                break;
            addrPos += 12;
            auto addrEnd = outsChunk.find("\"", addrPos);
            std::string addr = outsChunk.substr(addrPos, addrEnd - addrPos);

            auto amtPos = outsChunk.find("\"amount\":", addrEnd);
            amtPos += 9;
            auto amtEnd = outsChunk.find_first_of(",}", amtPos);
            double amt = std::stod(outsChunk.substr(amtPos, amtEnd - amtPos));

            outputs.emplace_back(amt, addr);
            cursor = amtEnd;
        }
    }

    UTXOTransaction tx(txId, inputs, outputs);
    return tx;
}

static Block parseBlockJson(const std::string &body)
{
    int index = std::stoi(extract(body, "\"index\":"));
    long long ts = std::stoll(extract(body, "\"timestamp\":"));
    int nonce = std::stoi(extract(body, "\"nonce\":"));
    int diff = std::stoi(extract(body, "\"difficulty\":"));
    std::string prev = extractQuoted(body, "\"previousHash\":\"");
    std::string hash = extractQuoted(body, "\"hash\":\"");

    // transactions 배열 파싱
    std::vector<UTXOTransaction> txs;
    size_t tArr = body.find("\"transactions\"");
    if (tArr != std::string::npos)
    {
        size_t tStart = body.find("[", tArr);
        size_t tEnd = body.find("]", tStart);
        std::string txChunk = body.substr(tStart + 1, tEnd - tStart - 1);
        size_t cursor = 0;
        while (true)
        {
            auto objStart = txChunk.find("{", cursor);
            if (objStart == std::string::npos)
                break;
            auto objEnd = txChunk.find("}", objStart);
            std::string oneTxJson = txChunk.substr(objStart, objEnd - objStart + 1);
            txs.push_back(parseTxJson(oneTxJson));
            cursor = objEnd + 1;
        }
    }

    Block blk(index, ts, txs, prev, nonce, diff);
    blk.setHash(hash); // 신뢰 모드: 수신 해시 사용
    return blk;
}

static std::string txToJson(const UTXOTransaction &tx)
{
    std::stringstream ss;
    ss << "{";
    ss << "\"tx_id\":\"" << tx.getId() << "\",";
    ss << "\"inputs\":[";
    const auto &inputs = tx.getInputs();
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        const auto &in = inputs[i];
        ss << "{";
        ss << "\"txId\":\"" << in.txId << "\",";
        ss << "\"outputIndex\":" << in.outputIndex << ",";
        ss << "\"signature\":\"" << in.signature << "\"";
        ss << "}";
        if (i < inputs.size() - 1)
            ss << ",";
    }
    ss << "],";
    ss << "\"outputs\":[";
    const auto &outs = tx.getOutputs();
    for (size_t i = 0; i < outs.size(); ++i)
    {
        const auto &out = outs[i];
        ss << "{";
        ss << "\"address\":\"" << out.address << "\",";
        ss << "\"amount\":" << out.amount;
        ss << "}";
        if (i < outs.size() - 1)
            ss << ",";
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

static std::string blockToJson(const Block &b)
{
    std::stringstream ss;
    ss << "{";
    ss << "\"index\":" << b.getIndex() << ",";
    ss << "\"timestamp\":" << b.getTimestamp() << ",";
    ss << "\"previousHash\":\"" << b.getPreviousHash() << "\",";
    ss << "\"hash\":\"" << b.getHash() << "\",";
    ss << "\"nonce\":" << b.getNonce() << ",";
    ss << "\"difficulty\":" << b.getDifficulty() << ",";
    ss << "\"transactions\":[";
    const auto &txs = b.getTransactions();
    for (size_t i = 0; i < txs.size(); ++i)
    {
        ss << txToJson(txs[i]);
        if (i < txs.size() - 1)
            ss << ",";
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

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

    // SSE stream for mining progress
    if (path.rfind("/mine/stream", 0) == 0 && method == "GET")
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
                std::string headers = "HTTP/1.1 200 OK\r\n";
                headers += "Content-Type: text/event-stream\r\n";
                headers += "Cache-Control: no-cache\r\n";
                headers += "Connection: keep-alive\r\n";
                headers += "Access-Control-Allow-Origin: *\r\n";
                headers += "Access-Control-Allow-Headers: Content-Type\r\n";
                headers += "\r\n";
                send(client_socket, headers.c_str(), headers.length(), 0);

                size_t lastAttempt = 0;
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));

                    std::vector<std::string> attempts;
                    bool done = false;
                    bool error = false;
                    std::string errMsg;
                    std::string hash;
                    int nonce = 0;
                    int difficulty = 0;

                    {
                        std::lock_guard<std::mutex> lk(job->mtx);
                        attempts = job->attempts;
                        done = job->done;
                        error = job->error;
                        errMsg = job->errMsg;
                        hash = job->hash;
                        nonce = job->nonce;
                        difficulty = job->difficulty;
                    }

                    if (attempts.size() == lastAttempt && !done && !error)
                        continue;

                    std::stringstream payload;
                    if (error)
                    {
                        payload << "{\"status\":\"error\",\"message\":\"" << errMsg << "\"}";
                    }
                    else
                    {
                        payload << "{\"status\":\"" << (done ? "done" : "running") << "\",";
                        if (done)
                        {
                            payload << "\"hash\":\"" << hash << "\",";
                            payload << "\"nonce\":" << nonce << ",";
                            payload << "\"difficulty\":" << difficulty << ",";
                        }
                        payload << "\"attempts\":[";
                        for (size_t i = 0; i < attempts.size(); ++i)
                        {
                            payload << "\"" << attempts[i] << "\"";
                            if (i < attempts.size() - 1)
                                payload << ",";
                        }
                        payload << "]}";
                    }

                    std::string event = "data: " + payload.str() + "\n\n";
                    if (send(client_socket, event.c_str(), event.length(), 0) <= 0)
                    {
                        break;
                    }

                    lastAttempt = attempts.size();
                    if (done || error)
                        break;
                }

                close(client_socket);
                return;
            }
        }
    }

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
    else if (path == "/utxos")
    {
        auto utxos = blockchain.getUTXOs();
        response_body = "[";
        for (size_t i = 0; i < utxos.size(); ++i)
        {
            const auto &[txId, index, output] = utxos[i];
            response_body += "{";
            response_body += "\"txId\":\"" + txId + "\",";
            response_body += "\"index\":" + std::to_string(index) + ",";
            response_body += "\"address\":\"" + output.address + "\",";
            response_body += "\"amount\":" + std::to_string(output.amount);
            response_body += "}";
            if (i < utxos.size() - 1)
                response_body += ",";
        }
        response_body += "]";
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
    else if (path == "/p2p/tx" && method == "POST")
    {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            std::string body = request.substr(body_start + 4);
            try
            {
                UTXOTransaction tx = parseTxJson(body);
                bool duplicate = false;
                for (const auto &p : blockchain.getPendingTransactions())
                {
                    if (p.getId() == tx.getId())
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                {
                    blockchain.addExternalPending(tx);
                }
                response_body = "{\"status\":\"ok\"}";
            }
            catch (const std::exception &e)
            {
                response_body = std::string("{\"status\":\"error\",\"message\":\"") + e.what() + "\"}";
            }
        }
    }
    else if (path == "/p2p/block" && method == "POST")
    {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            std::string body = request.substr(body_start + 4);
            try
            {
                Block b = parseBlockJson(body);
                if (blockchain.acceptExternalBlock(b))
                {
                    response_body = "{\"status\":\"ok\"}";
                }
                else
                {
                    response_body = "{\"status\":\"error\",\"message\":\"reject\"}";
                }
            }
            catch (const std::exception &e)
            {
                response_body = std::string("{\"status\":\"error\",\"message\":\"") + e.what() + "\"}";
            }
        }
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
                const auto &pending = blockchain.getPendingTransactions();
                if (!pending.empty())
                {
                    const auto &lastTx = pending.back();
                    broadcastJson("/p2p/tx", txToJson(lastTx));
                }
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
        blockchain.minePendingTransactions(miner, [&](const std::string &h, int n)
                                           {
            attempts.push_back(std::to_string(n) + ":" + h);
            if (attempts.size() > 50)
                attempts.erase(attempts.begin()); });
        blockchain.saveToFile(statePath);

        const Block &latest = blockchain.getLatestBlock();
        broadcastJson("/p2p/block", blockToJson(latest));
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

        std::thread([job, miner, statePath, &blockchain]()
                    {
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
                broadcastJson("/p2p/block", blockToJson(latest));
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
            } })
            .detach();

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
    int port = 8080;
    const char *envPort = std::getenv("PORT");
    if (envPort)
    {
        port = std::atoi(envPort);
    }
    address.sin_port = htons(port);

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

    std::cout << "Server listening on port " << port << "...\n";
    initPeersFromEnv();

    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            std::cerr << "Accept failed\n";
            continue;
        }
        std::thread([client_socket, &blockchain, statePath]()
                    { handleRequest(client_socket, blockchain, statePath); })
            .detach();
    }
}
