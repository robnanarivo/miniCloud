#pragma once

#include <string>
#include <vector>
#include <stdexcept>

struct NetworkInfo {
    std::string ip;
    unsigned short port;

    NetworkInfo();
    NetworkInfo(std::string ip, unsigned short port);
    NetworkInfo(std::string addr);

    // copy constructor
    NetworkInfo(const NetworkInfo &other);
    // assignment operator
    NetworkInfo& operator=(const NetworkInfo &other);
};