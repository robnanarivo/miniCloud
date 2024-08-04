#include "networkinfo.hpp"
#include "utils/utils.hpp"

#include <iostream>

NetworkInfo::NetworkInfo()
    : ip(""), port(0) {}

NetworkInfo::NetworkInfo(std::string ip, unsigned short port)
    : ip(ip), port(port) {}

NetworkInfo::NetworkInfo(std::string addr)
{
    std::vector<std::string> addr_info = split(addr, std::regex(":+"));
    if (addr_info.size() != 2) {
        throw std::invalid_argument("invalid network information");
    }
    ip = addr_info[0];
    port = static_cast<unsigned short>(std::stoul(addr_info[1]));
}

NetworkInfo::NetworkInfo(const NetworkInfo &other) 
    : ip(other.ip), port(other.port) {}

NetworkInfo& NetworkInfo::operator=(const NetworkInfo &other) 
{
    ip = other.ip;
    port = other.port;
    return *this;
}