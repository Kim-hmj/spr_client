#ifndef BROWSEMDNS_H
#define BROWSEMDNS_H

#include <string>
#include <vector>

enum IPVersion
{
    IPv4 = 0,
    IPv6 = 1
};


struct mDNSResult
{
    IPVersion ip_version;
    int iface_idx;
    std::string ip;
    std::string host;
    uint16_t port;
    bool valid;
};

class BrowsemDNS
{
public:
    virtual bool browse(const std::string& serviceName, mDNSResult& result, int timeout) = 0;
    virtual bool browse(const std::string& serviceName, const std::string& serviceType, const std::string& interfaceName, mDNSResult& result, int timeout) = 0;
    virtual bool browse(const std::string& serviceName, const std::string& serviceType, const std::string& interfaceName, std::vector<mDNSResult>& results, int timeout) = 0;
};


#include "browse_bonjour.hpp"
using BrowseZeroConf = BrowseBonjour;


#endif
