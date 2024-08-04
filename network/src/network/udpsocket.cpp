#include "udpsocket.hpp"

#include <stdexcept>
#include <iostream>

UDPSocket::UDPSocket()
    : UDPSocket(MAX_RECV_SIZE_DEFAULT) {}

UDPSocket::UDPSocket(size_t max_recv_size)
    : socket_fd_(-1),
      max_recv_size_(max_recv_size) {

    // create socket
    socket_fd_ = socket(PF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("UDPSocket: cannot create socket"); 
    }
}

UDPSocket::UDPSocket(UDPSocket&& other)
    : socket_fd_(std::exchange(other.socket_fd_, -1)),
      max_recv_size_(std::move(other.max_recv_size_)) {}

UDPSocket::UDPSocket(const UDPSocket& other)
    : socket_fd_(other.socket_fd_) {}

UDPSocket::~UDPSocket() {
    close(socket_fd_);
}

void UDPSocket::close_socket() {
    if (close(socket_fd_) == -1) {
        if (errno == EBADF) {
            std::cerr << "TCPSocket: WARNING: closing invalid socket" << std::endl;
        } else {
            throw std::runtime_error("TCPSocket: cannot close socket");
        }
    }
}

void UDPSocket::bind_port(unsigned short port, bool reuse) {
    // bind server port
    // set server socket info
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // reuse port
    if (reuse) {
        // configure socket to reuse address and port
        int socket_opt = 1;
        int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &socket_opt, sizeof(socket_opt));
        if (ret == -1) {
            throw std::runtime_error("UDPSocket: cannot set socket option");
        }
    }

    // bind port
    if (bind(socket_fd_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        throw std::runtime_error("UDPSocket: Cannot bind port " + std::to_string(port));
    }
}

void UDPSocket::send_raw(const unsigned char* data, size_t length,
                         const struct sockaddr_in& remote) {
    sendto(socket_fd_,
           data,
           length,
           0,
           (struct sockaddr*)&remote,
           sizeof(remote));
}

void UDPSocket::send_raw(const unsigned char* data, size_t length,
                         const std::string& remote_addr, const unsigned short port) {
    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    inet_pton(AF_INET, remote_addr.c_str(), &(dest.sin_addr));
    send_raw(data, length, dest);
}

void UDPSocket::send_raw(const unsigned char* data, size_t length,
                         const Address& address) {
    send_raw(data, length, address.hostname, address.port);
}

void UDPSocket::send_chunk(const DataChunk& chunk,
                           const struct sockaddr_in& remote) {
    send_raw(chunk.data.get(), chunk.size, remote);
}

void UDPSocket::send_chunk(const DataChunk& chunk,
                           const std::string& remote_addr, const unsigned short port) {
    send_raw(chunk.data.get(), chunk.size, remote_addr, port);
}

void UDPSocket::send_chunk(const DataChunk& chunk,
                           const Address& address) {
    send_raw(chunk.data.get(), chunk.size, address);
}

void UDPSocket::send_vec(const std::vector<unsigned char> vec,
                         const struct sockaddr_in& remote) {
    send_raw(vec.data(), vec.size(), remote);
}

void UDPSocket::send_vec(const std::vector<unsigned char> vec,
                         const std::string& remote_addr, const unsigned short port) {
    send_raw(vec.data(), vec.size(), remote_addr, port);
}

void UDPSocket::send_vec(const std::vector<unsigned char> vec,
                         const Address& address) {
    send_raw(vec.data(), vec.size(), address);
}

void UDPSocket::send_str(const std::string& str,
                         const struct sockaddr_in& remote) {
    send_raw(reinterpret_cast<const unsigned char*>(str.data()), str.size(), remote);
}
void UDPSocket::send_str(const std::string& str,
                         const std::string& remote_addr, const unsigned short port) {
    send_raw(reinterpret_cast<const unsigned char*>(str.data()), str.size(), remote_addr, port);
}
void UDPSocket::send_str(const std::string& str,
                         const Address& address) {
    send_raw(reinterpret_cast<const unsigned char*>(str.data()), str.size(), address);
}

std::pair<DataChunk, Address> UDPSocket::receive_chunk() {
    // construct source info
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);
    bzero(&src, srclen);

    // construct a big buffer
    std::unique_ptr<unsigned char[]> buf = std::make_unique<unsigned char[]>(max_recv_size_);
    size_t rlen = recvfrom(socket_fd_, buf.get(), max_recv_size_, 0, (struct sockaddr*)&src, &srclen);

    // construct a smaller aligned return data chunk
    DataChunk data_chunk(rlen);
    memcpy(data_chunk.data.get(), buf.get(), rlen);

    return {std::move(data_chunk), Address(inet_ntoa(src.sin_addr), ntohs(src.sin_port))};
}

std::pair<std::vector<unsigned char>, Address> UDPSocket::receive_vec() {
    auto res = receive_chunk();
    return {res.first.to_vec(), res.second};
}

std::pair<std::string, Address> UDPSocket::receive_str() {
    auto res = receive_chunk();
    return {res.first.to_str(), res.second};
}

int UDPSocket::get_socket() {
    return socket_fd_;
}
