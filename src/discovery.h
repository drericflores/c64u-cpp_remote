#pragma once

#include <string>
#include <vector>
#include <cstdint>   // for uint16_t

struct DiscoveredService {
    std::string hostname; // e.g., "C64U-01.local"
    std::string address;  // e.g., "10.0.0.183"
    uint16_t port = 0;
};

class DiscoveryService {
public:
    DiscoveryService();
    ~DiscoveryService();

    // Returns a list of discovered services via mDNS 
    std::vector<DiscoveredService> discoverMDNS(int timeoutMs = 800);

private:
    std::vector<DiscoveredService> results_;
};
