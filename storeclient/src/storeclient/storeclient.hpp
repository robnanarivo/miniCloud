#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>

#include "network/networkinfo.hpp"
#include "network/tcpsocket.hpp"
#include "utils/utils.hpp"

class StoreClient
{
private:
    NetworkInfo master_;
    // cache for row to IP and port
    std::unordered_map<std::string, NetworkInfo> addr_cache_;
    std::shared_mutex cache_mtx;
    NetworkInfo force_lookup(std::string row);

 public:
  StoreClient(NetworkInfo master);

  // ask the master about the ip and port of a tablet server that contains the
  // given row key
  NetworkInfo lookup_tablet(TabletKey key);

  // get data from a tablet given key
  ByteArray get(TabletKey key);

  // put data to a cell on tablet given key
  bool put(TabletKey key, ByteArray data);

  // put data to a cell on tablet given key and condition
  bool cput(TabletKey key, ByteArray old_data, ByteArray new_data);

  // delete data of a cell on tablet given key
  bool dele(TabletKey key);

  // send kill command to master
  bool kill_node(std::string tablet_addr);

  // send kill command to master
  bool rest_node(std::string tablet_addr);

  // send kill command to master
  std::vector<TabletKey> get_rows_cols(std::string tablet_addr);

  // get a list of backend nodes and their status
  std::string get_stats();

  // get raw data from a server
  ByteArray get_raw(std::string tablet_addr, TabletKey key);
};
