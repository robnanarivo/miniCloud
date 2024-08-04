#pragma once

#include <string>
#include <vector>

namespace Request {

// request to tabserver
enum class RequestType { get, put, cput, dele, chec, raw };
struct Request {
  std::string str;
  RequestType type;
  std::string row;
  std::string col;
  int v1_size;
  int v2_size;
  std::vector<char> v1;
  std::vector<char> v2;
  int tabid;
};

// request from secondary to primary, and back
enum class SecPriRequestType { init, hash, none, both, cp, lg };
struct SecPriRequest {
  SecPriRequestType type;
  int tabid;
  int cp_file_size;
  int lg_file_size;
};

// request from master to tabserver
enum class MaTabRequestType { add, init, kill, rest };
struct MaTabRequest {
  MaTabRequestType type;
  int tabid;
};

// request from admin console to master
enum class FeMaRequestType { look };
struct FeMaRequest {
  FeMaRequestType type;
  std::string row;
};

// request from admin console to master
enum class AmMaRequestType { kill, rest, stat };
struct AmMaRequest {
  AmMaRequestType type;
  std::string tabserver;
};

// request from tabserver to master
enum class TabMaRequestType { join, stat };
struct TabMaRequest {
  TabMaRequestType type;
  std::string tabserver;
};

};  // namespace Request