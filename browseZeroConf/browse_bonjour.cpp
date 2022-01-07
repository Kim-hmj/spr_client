#include "browse_bonjour.hpp"

#include <deque>
#include <iostream>
#include <memory>
#ifdef WINDOWS
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <net/if.h>
#include <map>
#endif

#include "common/aixlog.hpp"
#include "common/snap_exception.hpp"

using namespace std;

static constexpr auto LOG_TAG = "Bonjour";

struct DNSServiceRefDeleter
{
    void operator()(DNSServiceRef* ref)
    {
        DNSServiceRefDeallocate(*ref);
        delete ref;
    }
};

using DNSServiceHandle = std::unique_ptr<DNSServiceRef, DNSServiceRefDeleter>;

string BonjourGetError(DNSServiceErrorType error)
{
    switch (error)
    {
        case kDNSServiceErr_NoError:
            return "NoError";

        default:
        case kDNSServiceErr_Unknown:
            return "Unknown";

        case kDNSServiceErr_NoSuchName:
            return "NoSuchName";

        case kDNSServiceErr_NoMemory:
            return "NoMemory";

        case kDNSServiceErr_BadParam:
            return "BadParam";

        case kDNSServiceErr_BadReference:
            return "BadReference";

        case kDNSServiceErr_BadState:
            return "BadState";

        case kDNSServiceErr_BadFlags:
            return "BadFlags";

        case kDNSServiceErr_Unsupported:
            return "Unsupported";

        case kDNSServiceErr_NotInitialized:
            return "NotInitialized";

        case kDNSServiceErr_AlreadyRegistered:
            return "AlreadyRegistered";

        case kDNSServiceErr_NameConflict:
            return "NameConflict";

        case kDNSServiceErr_Invalid:
            return "Invalid";

        case kDNSServiceErr_Firewall:
            return "Firewall";

        case kDNSServiceErr_Incompatible:
            return "Incompatible";

        case kDNSServiceErr_BadInterfaceIndex:
            return "BadInterfaceIndex";

        case kDNSServiceErr_Refused:
            return "Refused";

        case kDNSServiceErr_NoSuchRecord:
            return "NoSuchRecord";

        case kDNSServiceErr_NoAuth:
            return "NoAuth";

        case kDNSServiceErr_NoSuchKey:
            return "NoSuchKey";

        case kDNSServiceErr_NATTraversal:
            return "NATTraversal";

        case kDNSServiceErr_DoubleNAT:
            return "DoubleNAT";

        case kDNSServiceErr_BadTime:
            return "BadTime";

        case kDNSServiceErr_BadSig:
            return "BadSig";

        case kDNSServiceErr_BadKey:
            return "BadKey";

        case kDNSServiceErr_Transient:
            return "Transient";

        case kDNSServiceErr_ServiceNotRunning:
            return "ServiceNotRunning";

        case kDNSServiceErr_NATPortMappingUnsupported:
            return "NATPortMappingUnsupported";

        case kDNSServiceErr_NATPortMappingDisabled:
            return "NATPortMappingDisabled";

        case kDNSServiceErr_NoRouter:
            return "NoRouter";

        case kDNSServiceErr_PollingMode:
            return "PollingMode";

        case kDNSServiceErr_Timeout:
            return "Timeout";
    }
}

struct mDNSReply
{
    string name, regtype, domain;
};

struct mDNSResolve
{
    string fullName;
    uint16_t port;
};

struct mDNSResolve_
{
    uint32_t ifIndex;
    string fullName;
    string host;
    uint16_t port;
    uint16_t txtLen;
    vector<char> txtRecord;
};

#define CHECKED(err)                                                                                                                                           \
    if ((err) != kDNSServiceErr_NoError)                                                                                                                       \
        throw SnapException(BonjourGetError(err) + ":" + to_string(__LINE__));

void runService(const DNSServiceHandle& service)
{
    if (!*service)
        return;

    auto socket = DNSServiceRefSockFD(*service);
    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket, &set);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    while (select(FD_SETSIZE, &set, NULL, NULL, &timeout))
    {
        CHECKED(DNSServiceProcessResult(*service));
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
    }
}

void runServiceWithTimeout(const DNSServiceHandle& service, double timeoutMs)
{
    if (!*service)
        return;

    auto socket = DNSServiceRefSockFD(*service);
    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket, &set);

    timeval timeout;
    timeout.tv_sec = (int)timeoutMs / 1000;
    timeout.tv_usec = (int)(timeoutMs*1000) % 1000000;

    while (select(FD_SETSIZE, &set, NULL, NULL, &timeout))
    {
        CHECKED(DNSServiceProcessResult(*service));
        timeout.tv_sec = (int)timeoutMs / 1000;
        timeout.tv_usec = (int)(timeoutMs*1000) % 1000000;
    }
}

bool getInterfaceNameIndex(std::map<unsigned int, std::string>& results)
{
    struct if_nameindex *head, *ifni;
    ifni = if_nameindex();
    head = ifni;

    if (head == NULL) {
        LOG(ERROR, LOG_TAG) << "if_nameindex()" << endl;
        return false;
    }

    while (ifni->if_index != 0) {
        results.insert(pair <unsigned int, std::string> (ifni->if_index, std::string(ifni->if_name)));
        LOG(NOTICE, LOG_TAG) << "Interface: " << ifni->if_index << " : " << ifni->if_name << endl;
        ifni++;
    }

    if_freenameindex(head);
    head = NULL;
    ifni = NULL;

    return true;
}

bool BrowseBonjour::browse(const string& serviceName, mDNSResult& result, int /*timeout*/)
{
    result.valid = false;
    // Discover
    deque<mDNSReply> replyCollection;
    {
        DNSServiceHandle service(new DNSServiceRef(NULL));
        CHECKED(DNSServiceBrowse(
            service.get(), 0, 0, serviceName.c_str(), "local.",
            [](DNSServiceRef /*service*/, DNSServiceFlags /*flags*/, uint32_t /*interfaceIndex*/, DNSServiceErrorType errorCode, const char* serviceName,
               const char* regtype, const char* replyDomain, void* context) {
                auto replyCollection = static_cast<deque<mDNSReply>*>(context);

                CHECKED(errorCode);
                replyCollection->push_back(mDNSReply{string(serviceName), string(regtype), string(replyDomain)});
            },
            &replyCollection));

        runService(service);
    }

    // Resolve
    deque<mDNSResolve> resolveCollection;
    {
        DNSServiceHandle service(new DNSServiceRef(NULL));
        for (auto& reply : replyCollection)
            CHECKED(DNSServiceResolve(
                service.get(), 0, 0, reply.name.c_str(), reply.regtype.c_str(), reply.domain.c_str(),
                [](DNSServiceRef /*service*/, DNSServiceFlags /*flags*/, uint32_t /*interfaceIndex*/, DNSServiceErrorType errorCode, const char* /*fullName*/,
                   const char* hosttarget, uint16_t port, uint16_t /*txtLen*/, const unsigned char* /*txtRecord*/, void* context) {
                    auto resultCollection = static_cast<deque<mDNSResolve>*>(context);

                    CHECKED(errorCode);
                    resultCollection->push_back(mDNSResolve{string(hosttarget), ntohs(port)});
                },
                &resolveCollection));

        runService(service);
    }

    // DNS/mDNS Resolve
    deque<mDNSResult> resultCollection(resolveCollection.size(), mDNSResult{IPVersion::IPv4, 0, "", "", 0, false});
    {
        DNSServiceHandle service(new DNSServiceRef(NULL));
        unsigned i = 0;
        for (auto& resolve : resolveCollection)
        {
            resultCollection[i].port = resolve.port;
            CHECKED(DNSServiceGetAddrInfo(
                service.get(), kDNSServiceFlagsLongLivedQuery, 0, kDNSServiceProtocol_IPv4, resolve.fullName.c_str(),
                [](DNSServiceRef /*service*/, DNSServiceFlags /*flags*/, uint32_t interfaceIndex, DNSServiceErrorType /*errorCode*/, const char* hostname,
                   const sockaddr* address, uint32_t /*ttl*/, void* context) {
                    auto result = static_cast<mDNSResult*>(context);

                    result->host = string(hostname);
                    result->ip_version = (address->sa_family == AF_INET) ? (IPVersion::IPv4) : (IPVersion::IPv6);
                    result->iface_idx = static_cast<int>(interfaceIndex);

                    char hostIP[NI_MAXHOST];
                    char hostService[NI_MAXSERV];
                    if (getnameinfo(address, sizeof(*address), hostIP, sizeof(hostIP), hostService, sizeof(hostService), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
                        result->ip = string(hostIP);
                    else
                        return;
                    result->valid = true;
                },
                &resultCollection[i++]));
        }
        runService(service);
    }

    resultCollection.erase(std::remove_if(resultCollection.begin(), resultCollection.end(), [](const mDNSResult& res) { return res.ip.empty(); }),
                           resultCollection.end());

    if (resultCollection.empty())
        return false;

    if (resultCollection.size() > 1)
        LOG(NOTICE, LOG_TAG) << "Multiple servers found.  Using first" << endl;

    result = resultCollection.front();

    return true;
}

bool BrowseBonjour::browse(const std::string& serviceName, const std::string& serviceType, const std::string& interfaceName, std::vector<mDNSResult>& results, int /*timeout*/)
{
    LOG(NOTICE, LOG_TAG) << " browse"<< endl;

    uint32_t interfaceIndex;
    if (interfaceName.empty()) {
        interfaceIndex = 0;
    } else {
        interfaceIndex = if_nametoindex(interfaceName.c_str());
        if (!interfaceIndex) {
            std::map<unsigned int, std::string> if_index_name;
            if (getInterfaceNameIndex(if_index_name)) {
                LOG(WARNING, LOG_TAG) << "avaliable interfaces: " << endl;
                for (auto& index_name : if_index_name) {
                    LOG(NOTICE, LOG_TAG) << index_name.first << " : " << index_name.second.c_str() << endl;
                }
            }
            return false;
        }
    }

    LOG(NOTICE, LOG_TAG) << "try to browse: <" << serviceName.c_str() << ">.<" << serviceType.c_str() << "><local.> interfaceName: <" << interfaceName.c_str() << "> interfaceIndex: <" << interfaceIndex << ">" << endl;
    // Discover
    deque<mDNSReply> replyCollection;
    {
        DNSServiceHandle service(new DNSServiceRef(NULL));
        CHECKED(DNSServiceBrowse(
            service.get(), 0, interfaceIndex, serviceType.c_str(), "local.",
            [](DNSServiceRef /*service*/, DNSServiceFlags /*flags*/, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char* replyName,
               const char* regtype, const char* replyDomain, void* context) {
                auto replyCollection = static_cast<deque<mDNSReply>*>(context);
                LOG(NOTICE) << "Browsed service: "<< replyName << "." << regtype << replyDomain << " InterfaceIndex: " << interfaceIndex << endl;
                CHECKED(errorCode);
                replyCollection->push_back(mDNSReply{string(replyName), string(regtype), string(replyDomain)});
            },
            &replyCollection));

        runServiceWithTimeout(service, 300);
    }

    // Remove
    deque<mDNSReply>::iterator it,it1;
    {
        // Remove the unexpected service
        if (!serviceName.empty()) {
            for (it = replyCollection.begin(); it != replyCollection.end();) {
                if (it->name != serviceName)
                    it = replyCollection.erase(it);
                else
                    it++;
            }
        }
        if (replyCollection.empty()) {
            LOG(ERROR) << "Couldn't find "<< serviceName.c_str() << "." << serviceType.c_str() << "local." << " on InterfaceIndex: " << interfaceIndex << endl;
            return false;
        }

        // Remove the repeated item of the same name
        for (it = ++replyCollection.begin(); it != replyCollection.end();) {
            for (it1 = replyCollection.begin(); it1 < it; it1++) {
                if (it1->name == it->name) {
                    break;
                }
            }
            if (it1 != it)
                it = replyCollection.erase(it);
            else
                it++;
        }
    }

    // Resolve
    deque<mDNSResolve_> resolveCollection;
    {
        DNSServiceHandle service(new DNSServiceRef(NULL));
        for (auto& reply : replyCollection) {
            LOG(NOTICE) << "Resoving : " << reply.name.c_str() << "." << reply.regtype.c_str() << reply.domain.c_str() << endl;
            CHECKED(DNSServiceResolve(
                service.get(), 0, interfaceIndex, reply.name.c_str(), reply.regtype.c_str(), reply.domain.c_str(),
                [](DNSServiceRef /*service*/, DNSServiceFlags /*flags*/, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char* fullName,
                   const char* hosttarget, uint16_t port, uint16_t txtLen, const unsigned char* txtRecord, void* context) {
                    auto resultCollection = static_cast<deque<mDNSResolve_>*>(context);

                    CHECKED(errorCode);
                    vector<char> txt(txtRecord, txtRecord+txtLen);
                    resultCollection->push_back(mDNSResolve_{interfaceIndex, string(fullName), string(hosttarget), ntohs(port), txtLen, txt});
                    LOG(NOTICE) << "Resoved service: Fullname: <" << fullName << "> Host: <" << hosttarget << "> port: <" << ntohs(port) << "> interfaceIndex: <" << interfaceIndex << ">" << endl;
                },
                &resolveCollection));

            runServiceWithTimeout(service, 300);
        }
    }

    for (int i = 0; i < resolveCollection.size(); i++) {
        LOG(NOTICE) << "resolve: <" << i+1 << ">"
        << "\nifaceindex: " << resolveCollection[i].ifIndex
        << "\nfullname  : " << resolveCollection[i].fullName.c_str()
        << "\nhost      : " << resolveCollection[i].host
        << "\nport      : " << resolveCollection[i].port
        << "\ntxtLen    : " << resolveCollection[i].txtLen
        << "\ntxtRecord : " << resolveCollection[i].txtRecord.data()
        << endl;
    }

    // DNS/mDNS Resolve
    deque<mDNSResult> resultCollection(resolveCollection.size(), mDNSResult{IPVersion::IPv4, 0, "", "", 0, false});
    {
        DNSServiceHandle service(new DNSServiceRef(NULL));
        unsigned i = 0;
        for (auto& resolve : resolveCollection)
        {
            resultCollection[i].port = resolve.port;
            LOG(NOTICE) << "DNS/mDNS Resoving. interfaceIndex: " << resolve.ifIndex << " host: " << resolve.host.c_str() << " fullName: " << resolve.fullName << endl;
            CHECKED(DNSServiceGetAddrInfo(
                service.get(), kDNSServiceFlagsLongLivedQuery, resolve.ifIndex, kDNSServiceProtocol_IPv4, resolve.host.c_str(),
                [](DNSServiceRef /*service*/, DNSServiceFlags /*flags*/, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char* hostname,
                   const sockaddr* address, uint32_t /*ttl*/, void* context) {
                    auto result = static_cast<mDNSResult*>(context);

                    CHECKED(errorCode);
                    result->host = string(hostname);
                    result->ip_version = (address->sa_family == AF_INET) ? (IPVersion::IPv4) : (IPVersion::IPv6);
                    result->iface_idx = static_cast<int>(interfaceIndex);

                    char hostIP[NI_MAXHOST];
                    char hostService[NI_MAXSERV];
                    if (getnameinfo(address, sizeof(*address), hostIP, sizeof(hostIP), hostService, sizeof(hostService), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                        result->ip = string(hostIP);
                        LOG(NOTICE) << "DNS resolved: hostname: " << hostname << " IP: " << hostIP << " interfaceIndex: " << interfaceIndex << " serveice: " << hostService << endl;
                    }
                    else {
                        LOG(ERROR) << "DNS resolve failed" << endl;
                        return;
                    }
                    result->valid = true;
                },
                &resultCollection[i++]));

            runServiceWithTimeout(service, 200);
        }
    }

    resultCollection.erase(std::remove_if(resultCollection.begin(), resultCollection.end(), [](const mDNSResult& res) { return res.ip.empty(); }),
                           resultCollection.end());

    if (resultCollection.empty())
        return false;

    //if (resultCollection.size() > 1)
    //    LOG(NOTICE, LOG_TAG) << "Multiple servers found.  Using first" << endl;

    //result = resultCollection.front();

    results.assign(resultCollection.begin(), resultCollection.end());
    LOG(NOTICE) << results.size() << " servers found." << endl;
    for (int i = 0; i < results.size(); i++) {
        LOG(NOTICE) << "result: <" << i+1 << ">"
        << "\nip_version: " << results[i].ip_version
        << "\nip        : " << results[i].ip
        << "\nhost      : " << results[i].host
        << "\nport      : " << results[i].port
        << "\nvalid     : " << results[i].valid
        << endl;
    }

    return true;
}

bool BrowseBonjour::browse(const std::string& serviceName, const std::string& serviceType, const std::string& interfaceName, mDNSResult& result, int /*timeout*/)
{
    std::vector<mDNSResult> results;
    if (browse(serviceName, serviceType, interfaceName, results, 0)) {
        if (results.size() >= 1) {
            if (results.size() > 1)
                LOG(NOTICE, LOG_TAG) << "Multiple servers found.  Using first" << endl;
            result = results.at(0);
            LOG(NOTICE) << "ip: " << result.ip.c_str() << endl;
            return true;
        }
    }

    return false;
}

#undef CHECKED
