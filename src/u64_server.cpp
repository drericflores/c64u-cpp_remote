#include "u64_server.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <curl/curl.h>

static size_t curlWriteToVec(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t n = size * nmemb;
    auto* v = reinterpret_cast<std::vector<uint8_t>*>(userdata);
    auto* b = reinterpret_cast<uint8_t*>(ptr);
    v->insert(v->end(), b, b + n);
    return n;
}

U64Server::U64Server(Creds creds) : creds_(std::move(creds)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (!creds_.address.empty() && creds_.address.back() == '/') creds_.address.pop_back();
    // quick connectivity check; don't throw on auth failure, but do throw on no connection
    (void)getVersion();
}

std::string U64Server::buildUrl(
    const std::string& path,
    const std::map<std::string, std::string>& params
) const {
    std::string url = creds_.address;
    if (!url.empty() && url.back() == '/') url.pop_back();

    if (!path.empty() && path.front() != '/') url += "/";
    url += path;

    if (!params.empty()) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        url += "?";
        bool first = true;
        for (const auto& kv : params) {
            char* ek = curl_easy_escape(curl, kv.first.c_str(), 0);
            char* ev = curl_easy_escape(curl, kv.second.c_str(), 0);
            if (!first) url += "&";
            first = false;
            url += (ek ? ek : kv.first.c_str());
            url += "=";
            url += (ev ? ev : kv.second.c_str());
            if (ek) curl_free(ek);
            if (ev) curl_free(ev);
        }
        curl_easy_cleanup(curl);
    }

    return url;
}

U64Server::HttpResult U64Server::request(
    const std::string& method,
    const std::string& path,
    const std::map<std::string, std::string>& params,
    const std::vector<uint8_t>* body,
    const std::string& contentType
) const {
    if (creds_.address.empty()) {
        throw std::runtime_error("No address set. Provide address in creds or use discovery.");
    }

    std::string url = buildUrl(path, params);

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    HttpResult out;

    // headers
    struct curl_slist* headers = nullptr;
    std::string pw = "X-Password: " + creds_.password;
    headers = curl_slist_append(headers, pw.c_str());
    if (!contentType.empty()) {
        std::string ct = "Content-Type: " + contentType;
        headers = curl_slist_append(headers, ct.c_str());
    }

    std::vector<uint8_t> response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToVec);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1500L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    if (method == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (body) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body->size()));
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        if (body) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body->size()));
        }
    }

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out.httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        throw std::runtime_error(std::string("HTTP request failed: ") + curl_easy_strerror(rc));
    }

    out.body = std::move(response);
    return out;
}

std::vector<uint8_t> U64Server::getVersion() {
    auto res = request("GET", "/v1/version", {}, nullptr, "");
    // Some firmwares may return 401/403 if password required; still means host is reachable.
    if (res.httpCode == 0) {
        throw std::runtime_error("No HTTP response code from /v1/version");
    }
    return res.body;
}

void U64Server::runPRG(const std::vector<uint8_t>& prgBytes) {
    auto res = request("POST", "/v1/runners:run_prg", {}, &prgBytes, "application/octet-stream");
    if (res.httpCode < 200 || res.httpCode >= 300) {
        std::string bodyStr(res.body.begin(), res.body.end());
        throw std::runtime_error("runPRG failed HTTP " + std::to_string(res.httpCode) + " body: " + bodyStr);
    }
}

std::vector<uint8_t> U64Server::peekMemory(uint16_t address, uint32_t length) {
    std::map<std::string, std::string> params;
    std::ostringstream a;
    a << std::hex;
    a.width(4);
    a.fill('0');
    a << address;

    params["address"] = a.str();
    params["length"] = std::to_string(length);

    auto res = request("GET", "/v1/machine:readmem", params, nullptr, "");
    if (res.httpCode < 200 || res.httpCode >= 300) {
        std::string bodyStr(res.body.begin(), res.body.end());
        throw std::runtime_error("peekMemory failed HTTP " + std::to_string(res.httpCode) + " body: " + bodyStr);
    }
    return res.body;
}

void U64Server::pokeMemory(uint16_t address, const std::vector<uint8_t>& data) {
    std::map<std::string, std::string> params;
    std::ostringstream a;
    a << std::hex;
    a.width(4);
    a.fill('0');
    a << address;

    params["address"] = a.str();

    auto res = request("POST", "/v1/machine:writemem", params, &data, "application/octet-stream");
    if (res.httpCode < 200 || res.httpCode >= 300) {
        std::string bodyStr(res.body.begin(), res.body.end());
        throw std::runtime_error("pokeMemory failed HTTP " + std::to_string(res.httpCode) + " body: " + bodyStr);
    }
}
