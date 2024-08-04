#include "address.hpp"

Address::Address(const std::string& s) {
    // parse hostname:port formatted string
    size_t colon_pos = s.find(':');
    hostname = s.substr(0, colon_pos);
    port = std::stoi(s.substr(colon_pos + 1, std::string::npos));
}

Address::Address(const std::string& hostname, unsigned short port)
    : hostname(hostname),
      port(port) {}

std::string Address::to_string() const {
    return hostname + ':' + std::to_string(port);
}

bool Address::operator==(const Address& other) const {
    return hostname == other.hostname && port == other.port;
}
