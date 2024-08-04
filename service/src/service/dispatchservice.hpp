#pragma once

#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <optional>

#include "network/address.hpp"
#include "network/udpsocket.hpp"

class DispatchService {
private:
    // map of known frontend servers -> their current reported load (#conns being handled)
    std::unordered_map<Address, unsigned> server_load_;

    // map of known frontend servers -> time of their last report
    std::unordered_map<Address, std::chrono::time_point<std::chrono::high_resolution_clock>> report_history_;
    
    // lock for the 2 maps
    std::mutex lock_;

    // UDP receive socket for receiving telemetry
    UDPSocket udp_socket_;

    // maintainance method - called in a daemon thread to cleanup outdated servers
    void cleanup_daemon_func();

    // maintainance method - called in a daemon thread to process heartbeat messages
    void heartbeat_daemon_func();

public:
    // Ctor
    DispatchService(unsigned short port);

    // get next frontend server
    std::optional<Address> get_frontend_server(bool random);

    // get current load
    std::unordered_map<Address, unsigned> get_server_load();
};
