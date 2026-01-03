#include "discovery.h"
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/simple-watch.h>
#include <thread>
#include <iostream>

static AvahiSimplePoll* simplePoll = nullptr;

static void resolve_callback(
    AvahiServiceResolver* r,
    AvahiIfIndex /*interface*/,
    AvahiProtocol /*protocol*/,
    AvahiResolverEvent event,
    const char* /*name*/,
    const char* /*type*/,
    const char* /*domain*/,
    const char* host_name,
    const AvahiAddress* address,
    uint16_t port,
    AvahiStringList* /*txt*/,
    AvahiLookupResultFlags /*flags*/,
    void* userdata)
{
    auto* out = static_cast<std::vector<DiscoveredService>*>(userdata);
    if (event == AVAHI_RESOLVER_FOUND) {
        char addrBuf[AVAHI_ADDRESS_STR_MAX];
        avahi_address_snprint(addrBuf, sizeof(addrBuf), address);
        DiscoveredService svc;
        svc.hostname = host_name ? host_name : "";
        svc.address = addrBuf;
        svc.port = port;
        out->push_back(svc);
    }
    avahi_service_resolver_free(r);
}

static void browse_callback(
    AvahiServiceBrowser* b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char* name,
    const char* type,
    const char* domain,
    AvahiLookupResultFlags /*flags*/,
    void* userdata)
{
    auto* services = static_cast<std::vector<DiscoveredService>*>(userdata);
    AvahiClient* client = avahi_service_browser_get_client(b);

    if (event == AVAHI_BROWSER_NEW) {
        avahi_service_resolver_new(
            client, interface, protocol,
            name, type, domain,
            AVAHI_PROTO_UNSPEC, AvahiLookupFlags(0),
            resolve_callback, services);
    }
}

DiscoveryService::DiscoveryService() = default;
DiscoveryService::~DiscoveryService() = default;

std::vector<DiscoveredService> DiscoveryService::discoverMDNS(int timeoutMs) {
    results_.clear();

    simplePoll = avahi_simple_poll_new();
    if (!simplePoll) {
        std::cerr << "Failed to create Avahi simple poll\n";
        return results_;
    }

    int error;
    AvahiClient* client = avahi_client_new(
        avahi_simple_poll_get(simplePoll),
        AvahiClientFlags(0), nullptr, nullptr, &error);

    if (!client) {
        std::cerr << "Avahi client error: " << avahi_strerror(error) << "\n";
        avahi_simple_poll_free(simplePoll);
        return results_;
    }

    const char* serviceType = "_http._tcp";
    AvahiServiceBrowser* browser = avahi_service_browser_new(
        client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
        serviceType, nullptr, AvahiLookupFlags(0),
        browse_callback, &results_);

    if (!browser) {
        std::cerr << "Failed to create Avahi browser\n";
        avahi_client_free(client);
        avahi_simple_poll_free(simplePoll);
        return results_;
    }

    std::thread pollThread([timeoutMs]() {
        avahi_simple_poll_loop(simplePoll);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
    avahi_simple_poll_quit(simplePoll);
    pollThread.join();

    avahi_service_browser_free(browser);
    avahi_client_free(client);
    avahi_simple_poll_free(simplePoll);

    return results_;
}
