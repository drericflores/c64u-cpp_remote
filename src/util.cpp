#include "util.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstring>   // memcpy
#include <cstdlib>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <curl/curl.h>

namespace util {

std::vector<uint8_t> readFileBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file: " + path);
    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    if (sz < 0) sz = 0;
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(sz));
    if (sz > 0) {
        f.read(reinterpret_cast<char*>(data.data()), sz);
        if (!f) throw std::runtime_error("Failed to read file: " + path);
    }
    return data;
}

static std::string slurpText(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Failed to open: " + path);
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

// Very small JSON "getter" that works for simple flat JSON like:
// { "address":"...", "password":"...", "enableMessageBox": false }
static std::string jsonGetString(const std::string& j, const std::string& key) {
    auto pos = j.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = j.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < j.size() && (j[pos] == ' ' || j[pos] == '\n' || j[pos] == '\r' || j[pos] == '\t')) pos++;
    if (pos >= j.size() || j[pos] != '"') return "";
    pos++;
    auto end = j.find('"', pos);
    if (end == std::string::npos) return "";
    return j.substr(pos, end - pos);
}

static bool jsonGetBool(const std::string& j, const std::string& key, bool defVal) {
    auto pos = j.find("\"" + key + "\"");
    if (pos == std::string::npos) return defVal;
    pos = j.find(':', pos);
    if (pos == std::string::npos) return defVal;
    pos++;
    while (pos < j.size() && (j[pos] == ' ' || j[pos] == '\n' || j[pos] == '\r' || j[pos] == '\t')) pos++;
    if (j.compare(pos, 4, "true") == 0) return true;
    if (j.compare(pos, 5, "false") == 0) return false;
    return defVal;
}

Creds loadCreds(const std::string& path) {
    Creds c;
    std::string j = slurpText(path);
    c.address = jsonGetString(j, "address");
    c.password = jsonGetString(j, "password");
    c.enableMessageBox = jsonGetBool(j, "enableMessageBox", false);

    // Normalize: allow bare IP and upgrade to http://
    if (!c.address.empty() && c.address.find("http://") != 0 && c.address.find("https://") != 0) {
        c.address = "http://" + c.address;
    }
    return c;
}

static size_t curlWriteToVec(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t n = size * nmemb;
    auto* v = reinterpret_cast<std::vector<uint8_t>*>(userdata);
    auto* b = reinterpret_cast<uint8_t*>(ptr);
    v->insert(v->end(), b, b + n);
    return n;
}

static bool httpProbeVersion(const std::string& baseUrl, int timeoutMs, std::string* outBody) {
    std::string url = baseUrl;
    if (!url.empty() && url.back() == '/') url.pop_back();
    url += "/v1/version";

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::vector<uint8_t> body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToVec);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) return false;
    if (code <= 0 || code >= 500) return false; // tolerate 401/403 etc? but C64U likely returns 200
    if (body.empty()) return false;

    if (outBody) outBody->assign(reinterpret_cast<const char*>(body.data()), body.size());
    return true;
}

// Convert network-order IPv4 (uint32) to dotted string
static std::string ipv4ToString(uint32_t n_net_order) {
    struct in_addr addr;
    addr.s_addr = n_net_order;
    char buf[INET_ADDRSTRLEN] = {0};
    const char* s = inet_ntop(AF_INET, &addr, buf, sizeof(buf));
    return s ? std::string(s) : std::string();
}

// Iterate host range: (ip & mask) + 1 .. (ip | ~mask) - 1 (typical)
static void enumerateHosts(uint32_t ip, uint32_t mask, std::vector<uint32_t>& out, int cap) {
    uint32_t net = ip & mask;
    uint32_t bcast = net | (~mask);

    // avoid /32
    if (net == bcast) return;

    uint32_t start = net + 1;
    uint32_t end = bcast - 1;
    int count = 0;

    for (uint32_t h = start; h <= end; ++h) {
        out.push_back(h);
        if (++count >= cap) break;
        if (h == 0xFFFFFFFFu) break;
    }
}

std::vector<FoundDevice> discoverU64(int timeoutMs, int maxHostsPerIface) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::vector<FoundDevice> found;

    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0 || !ifaddr) {
        return found;
    }

    for (auto* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;

        auto* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        auto* nm = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);
        if (!sa || !nm) continue;

        uint32_t ip = sa->sin_addr.s_addr;     // network order
        uint32_t mask = nm->sin_addr.s_addr;   // network order

        // Skip loopback 127.0.0.0/8
        uint32_t ip_host = ntohl(ip);
        if ((ip_host & 0xFF000000u) == 0x7F000000u) continue;

        // Enumerate up to cap per interface
        std::vector<uint32_t> hosts;
        enumerateHosts(ip, mask, hosts, maxHostsPerIface);

        for (uint32_t h_net : hosts) {
            std::string ipStr = ipv4ToString(h_net);
            if (ipStr.empty()) continue;

            std::string baseUrl = "http://" + ipStr;

            std::string body;
            if (httpProbeVersion(baseUrl, timeoutMs, &body)) {
                FoundDevice d;
                d.address = baseUrl;
                d.versionResponse = body;
                found.push_back(std::move(d));
            }
        }
    }

    freeifaddrs(ifaddr);
    return found;
}

int promptPickIndex(const std::vector<FoundDevice>& devs) {
    if (devs.empty()) return -1;
    if (devs.size() == 1) return 0;

    std::cout << "Multiple C64U-like devices found:\n";
    for (size_t i = 0; i < devs.size(); ++i) {
        std::cout << "  [" << i << "] " << devs[i].address << "\n";
    }
    std::cout << "Pick index: ";
    int idx = -1;
    std::cin >> idx;
    if (idx < 0 || static_cast<size_t>(idx) >= devs.size()) return -1;
    return idx;
}

} // namespace util
