#include "dispatchservice.hpp"
#include "utils/utils.hpp"

#include <thread>
#include <fstream>

DispatchService::DispatchService(unsigned short port)
    : server_load_(),
      report_history_(),
      lock_(),
      udp_socket_() {
    udp_socket_.bind_port(port);
    std::thread cleanup_daemon(&DispatchService::cleanup_daemon_func, this);
    cleanup_daemon.detach();
    std::thread heartbeat_daemon(&DispatchService::heartbeat_daemon_func, this);
    heartbeat_daemon.detach();
}

void DispatchService::cleanup_daemon_func() {
    while (true) {
        // sleep for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(6));
        // try to access maps
        lock_.lock();
        // get current
        auto curr_time = std::chrono::high_resolution_clock::now();
        for (const auto& record : report_history_) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - record.second);
            if (duration.count() > 6000) {
                // outdated frontend server info
                server_load_.erase(record.first);
                report_history_.erase(record.first);
            }
        }
        lock_.unlock();
    }
}

void DispatchService::heartbeat_daemon_func() {
    while (true) {
        auto msg_bundle = udp_socket_.receive_str();
        std::string msg = msg_bundle.first;
        if (msg == "LIST_LOAD") {
            // load request
            std::string response;
            for (const auto& load : get_server_load()) {
                response += load.first.to_string() + '=' + std::to_string(load.second) + ';';
            }
            udp_socket_.send_str(response, msg_bundle.second);
        } else {
            // heartbeat message
            auto tokens = tokenize(msg, ',');
            if (tokens.size() == 2 && is_integer(tokens[1])) {
                Address frontendserver(tokens[0]);
                lock_.lock();
                report_history_[frontendserver] = std::chrono::high_resolution_clock::now();
                server_load_[frontendserver] = std::stoi(tokens[1]);
                lock_.unlock();
            }
        }
    }
}

std::optional<Address> DispatchService::get_frontend_server(bool random) {
    lock_.lock();
    // no frontend servers available
    if (server_load_.size() == 0) {
        lock_.unlock();
        return {};
    }

    std::optional<Address> res{};

    if (!random) {
        // find one of the servers with least load
        auto it = std::min_element(server_load_.begin(), server_load_.end(),
                                [](const auto& l, const auto& r) {
                                        return l.second < r.second;
                                });
        // increment server's load
        it->second++;
        // copy the server's address
        res = {it->first};
    } else {
        // random find a frontend server
        auto random_it = server_load_.begin();
        std::advance(random_it, rand() % server_load_.size());
        res = {random_it->first};
    }
    lock_.unlock();
    return res;
}

std::unordered_map<Address, unsigned> DispatchService::get_server_load() {
    lock_.lock();
    auto res = server_load_;
    lock_.unlock();
    return res;
}
