#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <vector>

struct Settings {
    struct Server
    {
        std::string host{""};
        size_t port{1705};
    };

    size_t instance{1};
    std::string host_id;

    Server server;
};

#endif
