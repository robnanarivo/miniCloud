#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unistd.h>
#include "address.hpp"
#include "datachunk.hpp"
#include "httprequest.hpp"
#include "httpresponse.hpp"

#define TCP_BUFFER_SIZE_DEFAULT 1500

/*
To create a TCPSocket for a server:
>>>>>>>>>>>>>>>>>>
TCPSocket tcp_socket;
tcp_socket.bind_and_listen(8080, true);
while (true) {
    TCPSocket client_connection = tcp_socket.accept_connection();
    ... send and receive from client connection
}
<<<<<<<<<<<<<<<<<<

To create a TCPSocket for a client:
>>>>>>>>>>>>>>>>>>
TCPSocket tcp_socket;
tcp_socket.connect_to("www.google.com", 80);
... send and receive from server
<<<<<<<<<<<<<<<<<<

*/

enum TCP_SOCKET_MODE {
    TCP_SOCKET_CLIENT,
    TCP_SOCKET_SERVER,
    TCP_SOCKET_UNDEFINED,
};

class TCPSocket {
private:
    // mode of the socket
    TCP_SOCKET_MODE mode_;

    // the socket fd that this instance manages
    int socket_fd_;

    // buffer and its tracker
    // total buffer capacity
    size_t buffer_size_;
    // number of unprocessed chars in the buffer
    size_t buffer_count_;
    std::unique_ptr<unsigned char[]> buffer_;

    // metadata about this socket:
    // host internet info
    Address server_info_;

    // client internet info
    Address client_info_;

    // private helpers
    
    // check if a complete line is in buffer
    // return the index of '\r' + 1
    // return 0 if no line is found
    size_t contain_line();

    // read from the socket once and populate the buffer
    // the buffer may not be completely filled
    void fill_buffer();

public:
    // Ctor - a new socket will be created
    TCPSocket();

    // Ctor - overload: specify buffer size
    TCPSocket(size_t buffer_size);

    // CCtor
    TCPSocket(const TCPSocket& other);

    // Move Ctor
    TCPSocket(TCPSocket&& other);

    // Dtor - the socket will be closed
    ~TCPSocket();

    // manually close socket
    void close_socket();

    // getter for encapsulated filedescriptor
    int get_socket_fd() const;

    // getter for server and client info
    const Address& get_server_info() const;
    const Address& get_client_info() const;

    // SERVER ONLY
    // bind socket to certain port as server
    // set reuse = true to use this port continuously after restart
    void bind_and_listen(unsigned short port, bool reuse = true);

    // SERVER ONLY
    // accept a new connection and spawn a TCPSocket for that connection
    // blocks if no next incoming connection
    TCPSocket accept_connection();

    // CLIENT ONLY
    // connect to a remote TCP port
    void connect_to(const std::string& hostname, unsigned short port);
    void connect_to(const Address& remote);
    
    // called after connection established
    // either on a spawned client connection socket
    // or a client socket that has connected to a host socket

    // send chunk of bytes to the socket
    void send_chunk(const DataChunk& data_chunk);
    void send_vec(const std::vector<unsigned char>& bytes);
    void send_raw(const unsigned char* data, size_t length);

    // send a string
    void send_str(const std::string& str);

    // send an http response
    void send_http_res(const HTTPResponse& res);

    // receive chunk of bytes from the socket
    // need to specify #bytes to receive
    DataChunk receive_chunk(size_t length);
    std::vector<unsigned char> receive_vec(size_t length);

    // receive a string line from the socket
    std::string receive_line();

    // receive HTTP request from socket
    HTTPRequest receive_http_req();
};
