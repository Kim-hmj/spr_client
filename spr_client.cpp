#include <csignal>
//#include <adk/log.h>

#include <sys/resource.h>
//#include <boost/thread/thread.hpp> 
//#include <boost/thread/mutex.hpp>
#include <vector>
#include "browse_mdns.hpp"
#include "common/settings.hpp"
#include "common/str_compat.hpp"
#include "spr_client.h"
using namespace std;
//using boost::asio::ip::tcp;

//using namespace boost::asio;
Client::Client() {
}
Client::~Client() = default;

void Client::Start()
{
 if (settings_.server.host.empty())
    {
        browseMdns([this](/*const boost::system::error_code& ec,*/ const std::string& host, uint16_t port) {
            int a = 0;
            if (a != 0)
            {
                cout << "Failed to browse MDNS, error: " << a << "\n";
            } else {
                settings_.server.host = host;
                settings_.server.port = 1000;
                cout << "Found server " << settings_.server.host << ":" << settings_.server.port << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                connect(host);
            }
        });
    } else {
        cout << "server.host not empty()"<<endl;
    }
}
void Client::connect(string host)
{
    while(1){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        cout << "host__ = " << host << endl;
    }
    // io_service iosev;
    // ip::tcp::socket socket(iosev);
    // ip::tcp::endpoint ep(ip::address_v4::from_string(host), 1000);
    // boost::system::error_code ec;
    // socket.connect(ep,ec);
    // if(ec)
    // {
    //     std::cout << boost::system::system_error(ec).what() << std::endl;
    // }
    // char buf[100];
    // size_t len=socket.read_some(buffer(buf), ec);
    // std::cout.write(buf, len);
}
void Client::Stop()
{
    printf("%d: %s\n", __LINE__, __func__);
}

void Client::browseMdns(const MdnsHandler& handler)
{
    string sn_num = "hmj";
    try
    {
        BrowseZeroConf browser;
        mDNSResult avahiResult;
        if (browser.browse(string(sn_num), "_controller._tcp.","wlan0", avahiResult, 0))
        {
            string host = avahiResult.ip;
            uint16_t port = avahiResult.port;
            if (avahiResult.ip_version == IPVersion::IPv6)
                host += "%" + cpt::to_string(avahiResult.iface_idx);
                handler(host, port);
                return;
        }
    }
    catch (const std::exception& e)
    {
        cout << "Exception: " << e.what() << endl;
    }

}

int main(void)
{
        //boost::asio::io_context io_context;

        auto client = std::make_shared<Client>();
        client->Start();

}

