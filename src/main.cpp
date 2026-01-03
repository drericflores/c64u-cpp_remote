#include "u64_server.h"
#include "util.h"
#include "discovery.h"

#include <iostream>
#include <stdexcept>

static bool g_verbose = false;

static void usage() {
    std::cout <<
        "u64-remote [--creds /path/creds.json] [--address http://ip] [--password pw] "
        "[--discover] [--list] [--verbose] file.prg\n";
}

static void printDevices(const std::vector<DiscoveredService>& devs) {
    for (size_t i = 0; i < devs.size(); ++i) {
        std::cout << " [" << i << "] " << devs[i].hostname
                  << " (" << devs[i].address << ":" << devs[i].port << ")\n";
    }
}

static std::string getCachePath() {
    const char* home = std::getenv("HOME");
    if (!home) home = ".";
    return std::string(home) + "/.config/u64-remote/cache.json";
}

int main(int argc, char** argv) {
    try {
        std::string credsPath;
        std::string overrideAddr;
        std::string overridePw;
        bool discover = false;
        bool listOnly = false;
        std::string prgPath;

        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--creds" && i + 1 < argc) credsPath = argv[++i];
            else if (a == "--address" && i + 1 < argc) overrideAddr = argv[++i];
            else if (a == "--password" && i + 1 < argc) overridePw = argv[++i];
            else if (a == "--discover") discover = true;
            else if (a == "--list") listOnly = true;
            else if (a == "--verbose") g_verbose = true;
            else if (a == "-h" || a == "--help") { usage(); return 0; }
            else if (!a.empty() && a[0] == '-') { throw std::runtime_error("Unknown option: " + a); }
            else { prgPath = a; }
        }

        if (!listOnly && prgPath.empty()) {
            usage();
            return 2;
        }

        util::Creds c;
        bool loaded = false;
        auto tryLoad = [&](const std::string& p) -> bool {
            try { c = util::loadCreds(p); loaded = true; return true; }
            catch (...) { return false; }
        };

        if (!credsPath.empty()) {
            if (!tryLoad(credsPath)) throw std::runtime_error("Failed to load creds from: " + credsPath);
        } else {
            (void)tryLoad("creds.json");
            if (!loaded) (void)tryLoad("../json_examples/creds.json");
            if (!loaded) (void)tryLoad("json_examples/creds.json");
        }

        if (!overrideAddr.empty()) {
            c.address = overrideAddr;
            if (c.address.find("http://") != 0 && c.address.find("https://") != 0)
                c.address = "http://" + c.address;
        }
        if (!overridePw.empty()) c.password = overridePw;

        std::vector<DiscoveredService> devs;
        std::string cachePath = getCachePath();
        std::string cacheAddr, cacheHost;

        if (util::readCache(cachePath, cacheAddr, cacheHost)) {
            if (g_verbose) std::cout << "Checking cached device: " << cacheAddr << "\n";
            if (util::validateDevice(cacheAddr)) {
                if (!listOnly) {
                    c.address = cacheAddr; 
                    if (g_verbose) std::cout << "Using cached device: " << cacheAddr << "\n";
                } else {
                    std::cout << "Cached device: " << cacheHost << " (" << cacheAddr << ")\n";
                    return 0;
                }
            }
        }

        if (discover || c.address.empty() || listOnly) {
            if (g_verbose) std::cout << "Discovering devices via mDNS...\n";

            DiscoveryService disco;
            devs = disco.discoverMDNS(800);

            if (devs.empty()) {
                if (g_verbose) std::cout << "mDNS failed; subnet scanning...\n";
                devs = util::discoverU64(150, 256);
            }

            if (devs.empty()) {
                if (listOnly) { std::cout << "No devices found.\n"; return 0; }
                throw std::runtime_error("No devices discovered on network.");
            }

            if (listOnly) {
                std::cout << "Discovered devices:\n";
                printDevices(devs);
                return 0;
            }

            std::cout << "Select device index:\n";
            printDevices(devs);
            int idx; std::cin >> idx;
            if (idx < 0 || static_cast<size_t>(idx) >= devs.size())
                throw std::runtime_error("Invalid index selection");

            c.address = "http://" + devs[idx].address;
            util::writeCache(cachePath, c.address, devs[idx].hostname);
            if (g_verbose) std::cout << "Cached device: " << c.address << "\n";
        }

        if (c.password.empty() && g_verbose) std::cerr << "Warning: password is empty.\n";

        auto prgBytes = util::readFileBytes(prgPath);

        if (g_verbose) std::cout << "Uploading PRG (" << prgBytes.size() << " bytes) to " << c.address << "\n";

        U64Server::Creds sc;
        sc.address = c.address;
        sc.password = c.password;
        sc.enableMessageBox = c.enableMessageBox;

        U64Server server(sc);
        server.runPRG(prgBytes);

        std::cout << "Done.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
