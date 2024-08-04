#pragma once

#include <string>

// struct Address
struct Address {
    std::string hostname;
    unsigned short port;

    // deserialzation Ctor
    Address(const std::string& s);

    // hostname and port constructor
    Address(const std::string& hostname, unsigned short port);

    // serialization to string
    std::string to_string() const;

    // provide equal ability
    bool operator==(const Address& other) const;
};

// inject hash function for Address into namespace std
namespace std {
    template<>
    struct hash<Address> {
        size_t operator()(const Address& c) const {
            return std::hash<std::string>()(c.hostname) ^ std::hash<unsigned short>()(c.port);
        }
    };
}