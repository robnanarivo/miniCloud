#pragma once

#include <netinet/ip.h>

#include <string>

struct Address {
  sockaddr_in sock_addr;
  std::string sock_addr_str;
  std::string addr_str;
  int port;
};
