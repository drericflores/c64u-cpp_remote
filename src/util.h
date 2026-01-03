#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace util {

// Read entire file as bytes
std::vector<uint8_t> readFileBytes(const std::string& path);

// Simple, minimal JSON reader for creds.json (no external deps)
struct Creds {
    std::string address;   // e.g. http://10.0.0.183
    std::string password;  // API password
    bool enableMessageBox = false; // kept for parity (Linux: no-op)
};

Creds loadCreds(const std::string& path);

// Networking helpers for discovery
struct FoundDevice {
    std::string address; // http://x.x.x.x
    std::string versionResponse; // raw body from /v1/version
};

// Discover C64U devices on local subnets by probing /v1/version.
// maxHostsPerIface caps scanning; timeoutMs is per-host probe.
std::vector<FoundDevice> discoverU64(int timeoutMs = 250, int maxHostsPerIface = 512);

// Ask user to pick one device if more than one is found.
int promptPickIndex(const std::vector<FoundDevice>& devs);

} // namespace util
