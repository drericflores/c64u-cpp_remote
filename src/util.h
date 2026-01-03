#pragma once
#include "discovery.h"
#include <cstdint>
#include <string>
#include <vector>

namespace util {

// Read file bytes (for PRG)
std::vector<uint8_t> readFileBytes(const std::string& path);

// Credentials
struct Creds {
    std::string address;
    std::string password;
    bool enableMessageBox = false;
};

Creds loadCreds(const std::string& path);

// Fallback scanning (original subnet scan)
std::vector<DiscoveredService> discoverU64(int timeoutMs, int maxHostsPerIface);
int promptPickIndex(const std::vector<DiscoveredService>& devs);

// Cache
void writeCache(const std::string& path, const std::string& address, const std::string& hostname);
bool readCache(const std::string& path, std::string& address, std::string& hostname);
bool validateDevice(const std::string& address);

} // namespace util
