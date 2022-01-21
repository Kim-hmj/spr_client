#ifndef __Client_H_
#define __Client_H_
#include <iostream>
#include <chrono>
#include <string>
#include <map>
#include <memory>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
//#include <boost/asio.hpp>
#include <memory>
#include <algorithm>
#include "common/str_compat.hpp"
using namespace std;
using namespace std::chrono_literals;
class Client
{
public:
    Client();
    ~Client();

    void Start();
    void Stop();
    
private:
    using MdnsHandler = std::function<void(/*const boost::system::error_code& ec,*/ const std::string& host, uint16_t port)>;
    //using ResultHandler = std::function<void(const boost::system::error_code&)>;
    void browseMdns(const MdnsHandler& handler);
    void connect(string host);
    Settings settings_;
    
};


#endif
