#pragma once

#include "network/address.hpp"
#include "network/udpsocket.hpp"
#include "storeclient/storeclient.hpp"

class AdminService {
 private:
  StoreClient &storeClient_;
  Address dispatcher_;
  UDPSocket udpsocket_;

 public:
  AdminService(StoreClient &storeClient, const Address &dispatcher);
  std::unordered_map<std::string, bool> list_backend_server();
  std::vector<TabletKey> list_server_files(std::string server_addr);
  ByteArray get_raw(std::string server_addr, std::string row, std::string col);
  bool kill_server(std::string server_addr);
  bool reboot_server(std::string server_addr);
  std::unordered_map<Address, size_t> list_frontend_load();
};
