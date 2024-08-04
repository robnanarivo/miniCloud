#include "tcpsocket.hpp"
#include "utils/utils.hpp"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#include <string.h>
#include <stdexcept>


TCPSocket::TCPSocket()
    : TCPSocket(TCP_BUFFER_SIZE_DEFAULT) {}

TCPSocket::TCPSocket(size_t buffer_size)
    : mode_(TCP_SOCKET_UNDEFINED),
      socket_fd_(-1),
      buffer_size_(buffer_size),
      buffer_count_(0),
      buffer_(std::make_unique<unsigned char[]>(buffer_size)),
      server_info_("", 0),
      client_info_("", 0) {

    socket_fd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("TCPSocket: Cannot create socket");
    }
}

TCPSocket::TCPSocket(const TCPSocket& other)
    : mode_(other.mode_),
      socket_fd_(other.socket_fd_),
      buffer_size_(other.buffer_size_),
      buffer_count_(other.buffer_count_),
      buffer_(std::make_unique<unsigned char[]>(other.buffer_size_)),
      server_info_(other.server_info_),
      client_info_(other.client_info_) {

    memcpy(buffer_.get(), other.buffer_.get(), other.buffer_count_);
}

TCPSocket::TCPSocket(TCPSocket&& other)
    : mode_(std::exchange(other.mode_, TCP_SOCKET_UNDEFINED)),
      socket_fd_(std::exchange(other.socket_fd_, -1)),
      buffer_size_(std::move(other.buffer_size_)),
      buffer_count_(std::move(other.buffer_count_)),
      buffer_(std::move(other.buffer_)),
      server_info_(std::move(other.server_info_)),
      client_info_(std::move(other.client_info_)) {}

TCPSocket::~TCPSocket() {
    close(socket_fd_);
}

void TCPSocket::close_socket() {
    if (close(socket_fd_) == -1) {
        if (errno == EBADF) {
            std::cerr << "TCPSocket: WARNING: closing invalid socket" << std::endl;
        } else {
            throw std::runtime_error("TCPSocket: Cannot close socket");
        }
    }
}

int TCPSocket::get_socket_fd() const {
    return socket_fd_;
}

const Address& TCPSocket::get_server_info() const {
    return server_info_;
}

const Address& TCPSocket::get_client_info() const {
    return client_info_;
}

void TCPSocket::bind_and_listen(unsigned short port, bool reuse) {
    // check the mode of socket
    if (mode_ != TCP_SOCKET_UNDEFINED) {
        throw std::runtime_error("TCPSocket: Cannot bind and listen, wrong socket mode");
    }
    mode_ = TCP_SOCKET_SERVER;

    // bind to port
    // set server socket info
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // set port reuse
    if (reuse) {
        int socket_opt = 1;
        int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &socket_opt, sizeof(socket_opt));
        if (ret == -1) {
            throw std::runtime_error("TCPSocket: Cannot set socket to reuse addr & port");
        }
    }

    // bind port
    if (bind(socket_fd_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        throw std::runtime_error("TCPSocket: Cannot bind port " + std::to_string(port));
    }

    // listen to socket with backlog queue size = 1000
    if (listen(socket_fd_, 1000) == -1) {
        throw std::runtime_error("TCPSocket: Cannot listen to port " + std::to_string(port));
    }

    // update server_info_
    server_info_.hostname = inet_ntoa(servaddr.sin_addr);
    server_info_.port = ntohs(servaddr.sin_port);
}

TCPSocket TCPSocket::accept_connection() {
    if (mode_ != TCP_SOCKET_SERVER) {
        throw std::runtime_error("TCPSocket: accept_connection() cannot be called on a non-server socket");
    }
    // accept a single client
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    int comm_fd = accept(socket_fd_, (struct sockaddr*)&clientaddr, &clientaddrlen);
    
    // create copy of the main socket
    TCPSocket connection(*this);
    // modify the fields to make it new
    connection.socket_fd_ = comm_fd;
    connection.client_info_.hostname = inet_ntoa(clientaddr.sin_addr);
    connection.client_info_.port = ntohs(clientaddr.sin_port);
    return connection;
}

void TCPSocket::connect_to(const std::string& hostname, unsigned short port) {
    if (mode_ == TCP_SOCKET_SERVER) {
        throw std::runtime_error("TCPSocket: connect_to cannot be called on a non-client socket");
    }
    if (mode_ == TCP_SOCKET_UNDEFINED) {
        mode_ = TCP_SOCKET_CLIENT;
    }
    // try to connect to remote server
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, hostname.c_str(), &(servaddr.sin_addr));
    if (connect(socket_fd_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        switch (errno) {
        case ECONNREFUSED:
            throw std::runtime_error("TCPSocket: Connection to " + hostname + ':' + std::to_string(port) + " refused");
            break;
        case ENETUNREACH:
            throw std::runtime_error("TCPSocket: host " + hostname + " unreachable");
            break;
        default:
            throw std::runtime_error("TCPSocket: cannot connect to host " + hostname);
            break;
        }
    }

    // update remove server info
    server_info_.hostname = inet_ntoa(servaddr.sin_addr);
    server_info_.port = ntohs(servaddr.sin_port);
}

void TCPSocket::connect_to(const Address& remote) {
    connect_to(remote.hostname, remote.port);
}

void TCPSocket::send_chunk(const DataChunk& data_chunk) {
    send_raw(data_chunk.data.get(), data_chunk.size);
}

void TCPSocket::send_vec(const std::vector<unsigned char>& bytes) {
    send_raw(bytes.data(), bytes.size());
}

void TCPSocket::send_raw(const unsigned char* data, size_t length) {
    unsigned offset = 0;
    // loop write until everything's written
    while (offset < length) {
        // send string
        int n_bytes_read = send(socket_fd_,
                                data + offset,
                                length - offset,
                                MSG_NOSIGNAL);
        // error encountered
        if (n_bytes_read == -1) {
            if (errno == EINTR) {
                // send process interrupted
                continue;
            } else {
                throw std::runtime_error("TCPSocket: cannot write to socket " + std::to_string(socket_fd_));
            }
        }
        // increment offset
        offset += n_bytes_read;
    }
}

void TCPSocket::send_str(const std::string& str) {
    send_raw(reinterpret_cast<const unsigned char*>(str.data()), str.size());
    std::string trailer = "\r\n";
    send_raw(reinterpret_cast<const unsigned char*>(trailer.data()), 2);
}

void TCPSocket::send_http_res(const HTTPResponse& res) {
    // send first line
    send_str(res.protocol + ' ' + std::to_string(res.status_code) + ' ' + res.status_text);

    // send headers
    for (const auto& header : res.headers) {
        send_str(header.first + ": " + header.second);
    }
    send_str("");

    // send body
    send_chunk(res.body);
}

DataChunk TCPSocket::receive_chunk(size_t length) {
    // create buffer of size "length"
    DataChunk chunk(length);
    std::unique_ptr<unsigned char[]>& res = chunk.data;

    // grab valid bytes in buffer if exist
    size_t n_bytes_transfer = std::min<size_t>(buffer_count_, length);
    memcpy(res.get(), buffer_.get(), n_bytes_transfer);

    // consolidate buffer
    memmove(buffer_.get(), buffer_.get() + n_bytes_transfer, buffer_count_ - n_bytes_transfer);
    buffer_count_ -= n_bytes_transfer;

    // directly read the remaining bytes from socket
    size_t offset = n_bytes_transfer;
    while (offset < length) {
        size_t n_bytes_read = recv(socket_fd_,
                                   res.get() + offset,
                                   length - offset,
                                   0);
        if (n_bytes_read == 0) {
            throw std::runtime_error("TCPSocket: cannot receive, connection broken");
        }
        // if encounter error when reading
        if (n_bytes_read == -1) {
            // interrupted! read again will be fine
            if (errno == EINTR) {
                continue;
            } else {
                // encounter unexpected error, return -1
                throw std::runtime_error("TCPSocket: cannot read from socket " +
                                         std::to_string(socket_fd_) +
                                         " (" + strerror(errno) + ")");
            }
        }
        // update number of valid char in buffer
        offset += n_bytes_read;
    }
    return chunk;
}

std::vector<unsigned char> TCPSocket::receive_vec(size_t length) {
    return receive_chunk(length).to_vec();
}

std::string TCPSocket::receive_line() {
    while (contain_line() == 0) {
        // try the fill buffer if no line found in buffer
        fill_buffer();
    }
    size_t line_end = contain_line();
    
    // build the return string
    std::string res = "";
    for (size_t i = 0; i < line_end - 2; i++) {
        res += buffer_[i];
    }

    // move remaining valid chars to the front of the buffer
    memmove(buffer_.get(),
            buffer_.get() + line_end,
            buffer_count_ - line_end);
    buffer_count_ -= line_end;

    return res;
}

// private helpers

size_t TCPSocket::contain_line() {
    // try find CRLF
    for (size_t idx = 1; idx < buffer_count_; idx++) {
        if (buffer_[idx] == '\n' && buffer_[idx - 1] == '\r') {
            return idx + 1;
        }
    }
    // none found
    return 0;
}

void TCPSocket::fill_buffer() {
    // read from comm socket into remaining buffer
    // block unless socket is non-blocking!
    int n_bytes_read = recv(socket_fd_,
                            buffer_.get() + buffer_count_,
                            buffer_size_ - buffer_count_,
                            0);
    if (n_bytes_read == 0) {
        throw std::runtime_error("TCPSocket: cannot receive, connection broken");
    }
    // if encounter error when reading
    if (n_bytes_read == -1) {
        // interrupted! read again will be fine
        if (errno == EINTR) {
            return;
        } else {
            throw std::runtime_error("TCPSocket: cannot fill buffer from socket " +
                                        std::to_string(socket_fd_) +
                                        " (" + strerror(errno) + ")");
        }
    }
    // update number of valid char in buffer
    buffer_count_ += n_bytes_read;
}

HTTPRequest TCPSocket::receive_http_req() {

    // request to return
    HTTPRequest req;

    // try to find start line
    while (true) {
        const std::string line = receive_line();
        log_debug("TCPSocket: received line: \"" + line + "\"");
        // check #tokens
        auto tokens = tokenize(line, ' ');
        if (tokens.size() != 3) {
            // not start line, pathetic!
            log_debug("HTTPServer: Invalid line received: \"" + line + "\"");
            continue;
        }
        lower(tokens.at(0));

        // check HTTP method
        if (tokens[0] == "get") {
            req.method = HTTP_REQ_GET;
        } else if (tokens[0] == "post") {
            req.method = HTTP_REQ_POST;
        } else if (tokens[0] == "put") {
            req.method = HTTP_REQ_PUT;
        } else if (tokens[0] == "delete") {
            req.method = HTTP_REQ_DELETE;
        } else if (tokens[0] == "head") {
            req.method = HTTP_REQ_HEAD;
        } else if (tokens[0] == "options") {
            req.method = HTTP_REQ_OPTIONS;
        } else {
            // undefined method
            req.method = HTTP_REQ_OTHER;
        }

        // set target
        req.target = tokens.at(1);

        // set protocol
        req.protocol = tokens.at(2);
        break;
    }



    // try to find all headers
    while (true) {
        std::string line = receive_line();
        trim(line);
        lower(line);
        // split header key and value
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            // not a header line! quit loop
            break;
        }
        std::string header_key = line.substr(0, colon_pos);
        std::string header_value = line.substr(colon_pos + 1, std::string::npos);
        trim(header_value);

        // record header
        req.headers[header_key] = header_value;
    }

    // try to fill body
    if (req.headers.find("content-length") != req.headers.end() &&
        is_integer(req.headers.at("content-length"))) {
        // the sender has specified #bytes in the body, override
        size_t body_size = std::stoul(req.headers.at("content-length"));
        req.body = receive_chunk(body_size);
    } else if (req.headers.find("transfer-encoding") != req.headers.end() &&
               req.headers.at("transfer-encoding") == "chunked") {
        // chunked special body
        std::vector<unsigned char> body_buffer;
        while (true) {
            std::string chunk_size_line = receive_line();
            // watch out for parameters after a semicolon
            std::string chunk_size_str = chunk_size_line.substr(0, chunk_size_line.find(';'));
            std::istringstream iss(chunk_size_line);
            size_t chunk_size = 0;
            iss >> std::hex >> chunk_size;
            if (chunk_size == 0) {
                // final empty chunk
                receive_line();
                break;
            }
            // read #chunk_size into body buffer
            auto next_piece = receive_vec(chunk_size);
            // concatenate buffer
            body_buffer.insert(body_buffer.end(), next_piece.begin(), next_piece.end());
            // get rid of trailing CRLF
            receive_line();
        }
        // convert vector buffer into chunk
        req.body.data = std::make_unique<unsigned char[]>(body_buffer.size());
        memcpy(req.body.data.get(), body_buffer.data(), body_buffer.size());
    }

    // everything set, return request
    return req;
}
