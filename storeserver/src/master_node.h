#pragma once

#include <unordered_map>

#include "address.h"
#include "socket_reader_writer.h"

// struct for tabserver meta info
struct TabserverInfo {
  std::string id;
  std::vector<int> tabids;
  std::time_t last_heartbeat = 0;
  bool is_killed = false;
};

// Data structure for a master node
struct MasterNode {
  // address to bind, listen, and accept connections from frontend clients
  Address frontend_bind_addr;
  // connection from admin console
  Address admin_bind_addr;
  // connections from tabservers
  Address tabserver_bind_addr;

  // mapping tabserver to meta info
  std::unordered_map<std::string, TabserverInfo> tabservers;
  // mapping tabserver to its SRW
  std::unordered_map<std::string, SocketReaderWriter> tabserver_to_srw;

  // mapping tablet id to all tabservers that serve it
  std::unordered_map<int, std::vector<std::string>> tabid_to_tabservers;
};