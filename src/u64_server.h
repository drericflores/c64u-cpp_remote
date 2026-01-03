#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

class U64Server {
public:
    struct Creds {
        std::string address;   // base URL, e.g. http://10.0.0.183
        std::string password;  // API password header
        bool enableMessageBox = false; // parity field; Linux no-op
    };

    explicit U64Server(Creds creds);

    // GET /v1/version (connectivity check)
    std::vector<uint8_t> getVersion();

    // POST /v1/runners:run_prg (raw .prg bytes)
    void runPRG(const std::vector<uint8_t>& prgBytes);

    // GET /v1/machine:readmem?address=....&length=...
    std::vector<uint8_t> peekMemory(uint16_t address, uint32_t length);

    // POST /v1/machine:writemem?address=....
    void pokeMemory(uint16_t address, const std::vector<uint8_t>& data);

private:
    Creds creds_;

    struct HttpResult {
        long httpCode = 0;
        std::vector<uint8_t> body;
    };

    HttpResult request(
        const std::string& method,
        const std::string& path,
        const std::map<std::string, std::string>& params,
        const std::vector<uint8_t>* body,
        const std::string& contentType
    ) const;

    std::string buildUrl(
        const std::string& path,
        const std::map<std::string, std::string>& params
    ) const;
};
