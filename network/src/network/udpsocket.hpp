#pragma once

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <memory>
#include <vector>

#include "address.hpp"
#include "datachunk.hpp"

#define MAX_RECV_SIZE_DEFAULT 1500

class UDPSocket {
private:
    // socket FD
    int socket_fd_;

    // maximum receive message length
    size_t max_recv_size_;

public:
    // Ctor - 1.5 kilobytes maximum receive message length
    UDPSocket();

    // Ctor overload, specify max message size
    UDPSocket(size_t max_recv_size);

    // Move Ctor
    UDPSocket(UDPSocket&& other);

    // CCtor
    UDPSocket(const UDPSocket& other);

    // Dtor
    ~UDPSocket();

    // manual close socket
    void close_socket();

    // bind to a port
    void bind_port(unsigned short port, bool reuse = true);

    // send data to a remote node
    // send raw data
    void send_raw(const unsigned char* data, size_t length,
                  const struct sockaddr_in& remote);
    void send_raw(const unsigned char* data, size_t length,
                  const std::string& remote_addr, const unsigned short port);
    void send_raw(const unsigned char* data, size_t length,
                  const Address& address);
    
    // send DataChunk
    void send_chunk(const DataChunk& chunk,
                    const struct sockaddr_in& remote);
    void send_chunk(const DataChunk& chunk,
                    const std::string& remote_addr, const unsigned short port);
    void send_chunk(const DataChunk& chunk,
                    const Address& address);
    // send bytes vector
    void send_vec(const std::vector<unsigned char> vec,
                  const struct sockaddr_in& remote);
    void send_vec(const std::vector<unsigned char> vec,
                  const std::string& remote_addr, const unsigned short port);
    void send_vec(const std::vector<unsigned char> vec,
                  const Address& address);
    // send string
    void send_str(const std::string& str,
                  const struct sockaddr_in& remote);
    void send_str(const std::string& str,
                  const std::string& remote_addr, const unsigned short port);
    void send_str(const std::string& str,
                  const Address& address);

    // receive method
    std::pair<DataChunk, Address> receive_chunk();
    std::pair<std::vector<unsigned char>, Address> receive_vec();
    std::pair<std::string, Address> receive_str();

    // getter for socket FD
    int get_socket();
};