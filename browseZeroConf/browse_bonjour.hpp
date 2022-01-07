#ifndef BROWSEBONJOUR_H
#define BROWSEBONJOUR_H

#include <dns_sd.h>

class BrowseBonjour;

#include "browse_mdns.hpp"

class BrowseBonjour : public BrowsemDNS
{
public:
    bool browse(const std::string& serviceName, mDNSResult& result, int timeout) override;
    bool browse(const std::string& serviceName, const std::string& serviceType, const std::string& interfaceName, mDNSResult& result, int timeout) override;
    bool browse(const std::string& serviceName, const std::string& serviceType, const std::string& interfaceName, std::vector<mDNSResult>& results, int timeout) override;
};
#endif
