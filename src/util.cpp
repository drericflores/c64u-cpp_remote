#include "util.h"
#include "u64_server.h"
#include "discovery.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <filesystem>

// ---------------------
// readFileBytes
// ---------------------
std::vector<uint8_t> util::readFileBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file: " + path);
    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    if (sz < 0) sz = 0;
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> data((size_t)sz);
    if (sz > 0) {
        f.read(reinterpret_cast<char*>(data.data()), sz);
        if (!f) throw std::runtime_error("Failed to read file: " + path);
    }
    return data;
}

// ---------------------
// loadCreds
// ---------------------
static std::string slurpText(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Failed to open: " + path);
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

static std::string jsonGetString(const std::string& j, const std::string& key) {
    auto pos = j.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = j.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < j.size() && isspace(static_cast<unsigned char>(j[pos]))) pos++;
    if (pos >= j.size() || j[pos] != '"') return "";
    pos++;
    auto end = j.find('"', pos);
    if (end == std::string::npos) return "";
    return j.substr(pos, end - pos);
}

util::Creds util::loadCreds(const std::string& path) {
    std::string j = slurpText(path);
    util::Creds c;
    c.address = jsonGetString(j, "address");
    c.password = jsonGetString(j, "password");
    return c;
}

// ---------------------
// Cache
// ---------------------
static std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

void util::writeCache(const std::string& path, const std::string& address, const std::string& hostname) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream f(path);
    if (!f) return;
    f << "{\n";
    f << "  \"address\": \"" << escapeJson(address) << "\",\n";
    f << "  \"hostname\": \"" << escapeJson(hostname) << "\"\n";
    f << "}\n";
}

bool util::readCache(const std::string& path, std::string& address, std::string& hostname) {
    std::ifstream f(path);
    if (!f) return false;
    std::string content((std::istreambuf_iterator<char>(f)), {});
    auto findVal = [&](const std::string& key) -> std::string {
        auto p = content.find("\"" + key + "\"");
        if (p == std::string::npos) return "";
        p = content.find(":", p);
        if (p == std::string::npos) return "";
        p = content.find("\"", p);
        if (p == std::string::npos) return "";
        auto end = content.find("\"", p + 1);
        if (end == std::string::npos) return "";
        return content.substr(p + 1, end - (p + 1));
    };
    address = findVal("address");
    hostname = findVal("hostname");
    return !address.empty();
}

bool util::validateDevice(const std::string& address) {
    try {
        U64Server::Creds c;
        c.address = address;
        c.password = "";
        U64Server server(c);
        server.getVersion();
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------
// discoverU64
// ---------------------
std::vector<DiscoveredService> util::discoverU64(int timeoutMs, int /*maxHostsPerIface*/) {
    // Current discovery method uses mDNS/Avahi browsing & resolving.
    // maxHostsPerIface is preserved for API compatibility for potential future expansion.
    DiscoveryService d;
    return d.discoverMDNS(timeoutMs);
}
