#include "adminservice.hpp"

#include <iostream>

AdminService::AdminService(StoreClient& storeClient, const Address& dispatcher)
    : storeClient_(storeClient), dispatcher_(dispatcher), udpsocket_() {}

std::unordered_map<std::string, bool> AdminService::list_backend_server() {
  std::unordered_map<std::string, bool> results;

  std::string stats_str = storeClient_.get_stats();
  std::vector<std::string> nodes_stats = split(stats_str, std::regex("\\s+"));

  for (auto itr = nodes_stats.begin(); itr != nodes_stats.end(); ++itr) {
    std::vector<std::string> node_stat = split(*itr, std::regex("="));
    if (node_stat.size() != 2) {
      throw std::invalid_argument("Cannot parse server status");
    }
    if (node_stat[1] == "0") {
      results.insert({node_stat[0], false});
    } else {
      results.insert({node_stat[0], true});
    }
  }

  return results;
}

std::vector<TabletKey> AdminService::list_server_files(
    std::string server_addr) {
  return storeClient_.get_rows_cols(server_addr);
}

ByteArray AdminService::get_raw(std::string server_addr, std::string row,
                                std::string col) {
  TabletKey key(row, col);
  return storeClient_.get_raw(server_addr, key);
}

bool AdminService::kill_server(std::string server_addr) {
  return storeClient_.kill_node(server_addr);
}

bool AdminService::reboot_server(std::string server_addr) {
  return storeClient_.rest_node(server_addr);
}

std::unordered_map<Address, size_t> AdminService::list_frontend_load() {
  udpsocket_.send_str("LIST_LOAD", dispatcher_);
  auto msg = udpsocket_.receive_str().first;
  log_debug("AdminService: received message from dispatcher: \"" + msg + "\"");
  std::unordered_map<Address, size_t> res;
  auto loads = tokenize(msg, ';');
  for (const auto& load : loads) {
    auto load_pair = tokenize(load, '=');
    if (load_pair.size() == 2 && is_integer(load_pair.at(1))) {
      res[Address(load_pair.at(0))] = std::stoul(load_pair.at(1));
    }
  }
  return res;
}
