#include "storeclient.hpp"

#include <stdexcept>

StoreClient::StoreClient(NetworkInfo master) : master_(master) {}

NetworkInfo StoreClient::force_lookup(std::string row) {
  TCPSocket tcp;
  tcp.connect_to(master_.ip, master_.port);
  tcp.send_str("LOOK " + row);
  NetworkInfo tablet_addr = NetworkInfo(tcp.receive_line());

  std::unique_lock write_lock(cache_mtx);
  addr_cache_[row] = tablet_addr;

  return tablet_addr;
}

NetworkInfo StoreClient::lookup_tablet(TabletKey key) {
  std::string row = key.row;
  // use cache when possible
  std::shared_lock read_lock(cache_mtx);
  if (addr_cache_.find(row) != addr_cache_.end()) {
    return addr_cache_[row];
  }
  read_lock.unlock();

  return force_lookup(row);
}

ByteArray StoreClient::get(TabletKey key) {
  NetworkInfo tablet_addr = lookup_tablet(key);
  TCPSocket tcp;
  try {
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  } catch (const std::exception &e) {
    tablet_addr = force_lookup(key.row);
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  }
  tcp.send_str("GET " + key.row + " " + key.col);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    // if get failed, throw an exception
    throw std::runtime_error("Given key does not exist in storage");
  }
  size_t data_size = std::stoul(res[1]);
  ByteArray data = tcp.receive_vec(data_size);
  return data;
}

bool StoreClient::put(TabletKey key, ByteArray data) {
  NetworkInfo tablet_addr = lookup_tablet(key);
  TCPSocket tcp;
  try {
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  } catch (const std::exception &e) {
    tablet_addr = force_lookup(key.row);
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  }
  tcp.send_str("PUT " + key.row + " " + key.col + " " +
               std::to_string(data.size()));
  tcp.send_vec(data);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    return false;
  }
  return true;
}

bool StoreClient::cput(TabletKey key, ByteArray old_data, ByteArray new_data) {
  NetworkInfo tablet_addr = lookup_tablet(key);
  TCPSocket tcp;
  try {
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  } catch (const std::exception &e) {
    tablet_addr = force_lookup(key.row);
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  }
  tcp.send_str("CPUT " + key.row + " " + key.col + " " +
               std::to_string(old_data.size()) + " " +
               std::to_string(new_data.size()));
  tcp.send_vec(old_data);
  tcp.send_vec(new_data);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    return false;
  }
  return true;
}

bool StoreClient::dele(TabletKey key) {
  NetworkInfo tablet_addr = lookup_tablet(key);
  TCPSocket tcp;
  try {
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  } catch (const std::exception &e) {
    tablet_addr = force_lookup(key.row);
    tcp.connect_to(tablet_addr.ip, tablet_addr.port);
  }
  tcp.send_str("DELE " + key.row + " " + key.col);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    return false;
  }
  return true;
}

bool StoreClient::kill_node(std::string tablet_addr) {
  TCPSocket tcp;
  tcp.connect_to(master_.ip, master_.port);
  tcp.send_str("KILL " + tablet_addr);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    return false;
  }
  return true;
}

bool StoreClient::rest_node(std::string tablet_addr) {
  TCPSocket tcp;
  tcp.connect_to(master_.ip, master_.port);
  tcp.send_str("REST " + tablet_addr);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    return false;
  }
  return true;
}

std::vector<TabletKey> StoreClient::get_rows_cols(std::string tablet_addr) {
  std::vector<TabletKey> results;
  NetworkInfo server(tablet_addr);
  TCPSocket tcp;
  tcp.connect_to(server.ip, server.port);
  tcp.send_str("RAW");
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  int num_pairs = std::atoi(res[1].c_str());
  for (int i = 0; i < num_pairs; ++i) {
    std::vector<std::string> pair =
        split(tcp.receive_line(), std::regex("\\s+"));
    results.push_back(TabletKey(pair[0], pair[1]));
  }
  return results;
}

std::string StoreClient::get_stats() {
  TCPSocket tcp;
  tcp.connect_to(master_.ip, master_.port);
  tcp.send_str("STAT");
  return tcp.receive_line();
}

ByteArray StoreClient::get_raw(std::string tablet_addr, TabletKey key) {
  NetworkInfo server(tablet_addr);
  TCPSocket tcp;
  tcp.connect_to(server.ip, server.port);
  tcp.send_str("GET " + key.row + " " + key.col);
  std::vector<std::string> res = split(tcp.receive_line(), std::regex("\\s+"));
  if (res[0] != "+OK") {
    // if get failed, throw an exception
    throw std::runtime_error("Given key does not exist in storage");
  }
  size_t data_size = std::stoul(res[1]);
  ByteArray data = tcp.receive_vec(data_size);
  return data;
}
