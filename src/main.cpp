#include "u64_server.h"
#include "util.h"

#include <iostream>
#include <stdexcept>

static void usage() {
    std::cout <<
        "u64-remote [--creds /path/creds.json] [--address http://ip] [--password pw] [--discover] file.prg\n";
}

int main(int argc, char** argv) {
    try {
        std::string credsPath;
        std::string overrideAddr;
        std::string overridePw;
        bool discover = false;
        std::string prgPath;

        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--creds" && i + 1 < argc) {
                credsPath = argv[++i];
            } else if (a == "--address" && i + 1 < argc) {
                overrideAddr = argv[++i];
            } else if (a == "--password" && i + 1 < argc) {
                overridePw = argv[++i];
            } else if (a == "--discover") {
                discover = true;
            } else if (a == "-h" || a == "--help") {
                usage();
                return 0;
            } else if (!a.empty() && a[0] == '-') {
                throw std::runtime_error("Unknown option: " + a);
            } else {
                prgPath = a;
            }
        }

        if (prgPath.empty()) {
            usage();
            return 2;
        }

        // Default creds path search order:
        // 1) --creds
        // 2) ./creds.json
        // 3) ../json_examples/creds.json (your stated location)
        // 4) ./json_examples/creds.json
        // If nothing found, we proceed with empty creds and rely on --discover/--address.
        util::Creds c;
        bool loaded = false;

        auto tryLoad = [&](const std::string& p) -> bool {
            try {
                c = util::loadCreds(p);
                loaded = true;
                return true;
            } catch (...) {
                return false;
            }
        };

        if (!credsPath.empty()) {
            if (!tryLoad(credsPath)) {
                throw std::runtime_error("Failed to load creds from: " + credsPath);
            }
        } else {
            (void)tryLoad("creds.json");
            if (!loaded) (void)tryLoad("../json_examples/creds.json");
            if (!loaded) (void)tryLoad("json_examples/creds.json");
        }

        // Overrides
        if (!overrideAddr.empty()) {
            c.address = overrideAddr;
            if (c.address.find("http://") != 0 && c.address.find("https://") != 0) {
                c.address = "http://" + c.address;
            }
        }
        if (!overridePw.empty()) c.password = overridePw;

        // Discovery if requested or address is empty
        if (discover || c.address.empty()) {
            std::cout << "Discovering C64U on local network...\n";
            auto devs = util::discoverU64(250, 512);
            if (devs.empty()) {
                throw std::runtime_error("No C64U devices discovered. Provide --address or set address in creds.json.");
            }
            int idx = util::promptPickIndex(devs);
            if (idx < 0) throw std::runtime_error("No device selected.");
            c.address = devs[(size_t)idx].address;
            std::cout << "Using: " << c.address << "\n";
        }

        if (c.password.empty()) {
            std::cerr << "Warning: password is empty. If your C64U requires one, requests may fail.\n";
        }

        // Load PRG file
        auto prg = util::readFileBytes(prgPath);

        // Connect & run
        U64Server::Creds sc;
        sc.address = c.address;
        sc.password = c.password;
        sc.enableMessageBox = c.enableMessageBox;

        U64Server server(sc);

        std::cout << "Uploading PRG (" << prg.size() << " bytes) ...\n";
        server.runPRG(prg);
        std::cout << "Done.\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
