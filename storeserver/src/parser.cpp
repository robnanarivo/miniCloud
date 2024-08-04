#include "parser.h"

#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <thread>

#include "db_logger.h"
#include "master_node.h"
#include "tablet_node.h"

// regex definitions
const static std::regex addr_regex{"^([0-9.]+):([0-9]+)$"};
const static std::regex get_regex("^GET (.*) (.*)\r\n$");
const static std::regex put_regex("^PUT (.*) (.*) ([0-9]+)\r\n$");
const static std::regex cput_regex("^CPUT (.*) (.*) ([0-9]+) ([0-9]+)\r\n$");
const static std::regex dele_regex("^DELE (.*) (.*)\r\n$");
const static std::regex chec_regex("^CHEC ([0-9]+)\r\n$");
const static std::regex raw_regex("^RAW\r\n$");

const static std::regex secpri_init_regex("^INIT\r\n$");
const static std::regex secpri_hash_regex("^HASH ([0-9]+)\r\n$");
const static std::regex secpri_none_regex("^NONE ([0-9]+)\r\n$");
const static std::regex secpri_both_regex(
    "^BOTH ([0-9]+) ([0-9]+) ([0-9]+)\r\n$");
const static std::regex secpri_cp_regex("^CP ([0-9]+) ([0-9]+)\r\n$");
const static std::regex secpri_lg_regex("^LG ([0-9]+) ([0-9]+)\r\n$");

const static std::regex matab_add_regex("^ADD ([0-9]+)\r\n$");
const static std::regex matab_init_regex("^INIT\r\n$");
const static std::regex matab_kill_regex("^KILL\r\n$");
const static std::regex matab_rest_regex("^REST\r\n$");

const static std::regex fema_look_regex("^LOOK (.*)\r\n$");

const static std::regex amma_kill_regex("^KILL (.*)\r\n$");
const static std::regex amma_rest_regex("^REST (.*)\r\n$");
const static std::regex amma_stat_regex("^STAT\r\n$");

const static std::regex tabma_join_regex("^JOIN (.*)\r\n$");
const static std::regex tabma_stat_regex("^STAT (.*)\r\n$");

// parse ip address and port from a string
Address Parser::parse_addr(std::string sock_addr_str) {
  // parse address string
  std::smatch base_match;
  std::regex_match(sock_addr_str, base_match, addr_regex);

  // construct Address
  Address result;
  result.sock_addr_str = sock_addr_str;
  result.addr_str = base_match[1].str();
  result.port = std::atoi(base_match[2].str().c_str());

  bzero(&result.sock_addr, sizeof(result.sock_addr));
  result.sock_addr.sin_family = AF_INET;
  result.sock_addr.sin_port = htons(result.port);
  inet_pton(AF_INET, result.addr_str.c_str(), &(result.sock_addr.sin_addr));

  return result;
}

// parse a sockaddr into string representation
std::string Parser::parse_sockaddr(const sockaddr_in &src) {
  char src_text[20];
  inet_ntop(AF_INET, &src.sin_addr, src_text, 20);
  int src_port = ntohs(src.sin_port);
  std::string src_addr(src_text);
  src_addr.append(":").append(std::to_string(src_port));

  return src_addr;
}

// parse tabserver configuration file
void Parser::parse_config(const std::string &config_file, const int &index,
                          TabletNode &tablet_node) {
  // open file for reading
  std::ifstream istrm(config_file, std::ios_base::in);

  // extract master and admin addresses
  std::string master_str;
  istrm >> master_str;

  tablet_node.master_addr = Parser::parse_addr(master_str);

  std::string secondary_primary_conn_addr;
  std::string frontend_bind_addr;
  istrm >> secondary_primary_conn_addr >> frontend_bind_addr;

  if (index == 1) {
    // this is primary node
    tablet_node.is_primary = true;
    // addr to accept frontend connections, and to listen for secondary
    // connection
    tablet_node.frontend_bind_addr = Parser::parse_addr(frontend_bind_addr);
    tablet_node.secondary_primary_conn_addr =
        Parser::parse_addr(secondary_primary_conn_addr);
  } else {
    // this is secondary node
    tablet_node.is_primary = false;
    // addr to forward update requests, and to connect to primary
    tablet_node.primary_addr = Parser::parse_addr(frontend_bind_addr);
    tablet_node.secondary_primary_conn_addr =
        Parser::parse_addr(secondary_primary_conn_addr);

    // find and add this node's frontend and primary bind address
    for (int counter = 2; istrm >> frontend_bind_addr; ++counter) {
      if (counter == index) {
        tablet_node.frontend_bind_addr = Parser::parse_addr(frontend_bind_addr);
        break;
      }
    }
  }

  istrm.close();
}

// parse master configuration file
bool Parser::parse_master_config(const std::string &config_file,
                                 MasterNode &master_node) {
  std::ifstream istrm(config_file, std::ios_base::in);

  // extract bind addresses
  std::string frontend_bind_addr;
  std::string admin_bind_addr;
  std::string tabserver_bind_addr;
  istrm >> frontend_bind_addr >> admin_bind_addr >> tabserver_bind_addr;

  master_node.frontend_bind_addr = Parser::parse_addr(frontend_bind_addr);
  master_node.admin_bind_addr = Parser::parse_addr(admin_bind_addr);
  master_node.tabserver_bind_addr = Parser::parse_addr(tabserver_bind_addr);

  // extract tablet server addresses in clusters
  std::string tabserver_id_1;
  std::string tabserver_id_2;
  std::string tabserver_id_3;
  for (int cluster = 1;
       istrm >> tabserver_id_1 >> tabserver_id_2 >> tabserver_id_3; ++cluster) {
    // these three tabservers serve the same tabids
    std::vector<std::string> tabservers = {tabserver_id_1, tabserver_id_2,
                                           tabserver_id_3};
    // extract tabids served by this cluster
    std::vector<int> tabids;
    for (int i = 0; i < 3; ++i) {
      int tabid;
      istrm >> tabid;
      tabids.push_back(tabid);
      master_node.tabid_to_tabservers.insert({tabid, tabservers});
    }

    TabserverInfo tabserver_1{tabserver_id_1, tabids};
    TabserverInfo tabserver_2{tabserver_id_2, tabids};
    TabserverInfo tabserver_3{tabserver_id_3, tabids};

    master_node.tabservers.insert({tabserver_id_1, tabserver_1});
    master_node.tabservers.insert({tabserver_id_2, tabserver_2});
    master_node.tabservers.insert({tabserver_id_3, tabserver_3});
  }

  istrm.close();
  return true;
}

// parse request from frontend clients to tabservers
bool Parser::parse_request(const std::string &line, Request::Request &request) {
  std::smatch base_match;
  if (std::regex_match(line, base_match, get_regex)) {
    request.str = line;
    request.type = Request::RequestType::get;
    request.row = base_match[1].str();
    request.col = base_match[2].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a GET request row <" << request.row << "> col <"
        << request.col << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, put_regex)) {
    request.str = line;
    request.type = Request::RequestType::put;
    request.row = base_match[1].str();
    request.col = base_match[2].str();
    request.v1_size = std::atoi(base_match[3].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a PUT request row <" << request.row << "> col <"
        << request.col << "> size <" << request.v1_size << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, cput_regex)) {
    request.str = line;
    request.type = Request::RequestType::cput;
    request.row = base_match[1].str();
    request.col = base_match[2].str();
    request.v1_size = std::atoi(base_match[3].str().c_str());
    request.v2_size = std::atoi(base_match[4].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a CPUT request row <" << request.row << "> col <"
        << request.col << "> v1_size <" << request.v1_size << "> v2_size <"
        << request.v2_size << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, dele_regex)) {
    request.str = line;
    request.type = Request::RequestType::dele;
    request.row = base_match[1].str();
    request.col = base_match[2].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a DELE request row <" << request.row << "> col <"
        << request.col << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, chec_regex)) {
    request.str = line;
    request.type = Request::RequestType::chec;
    request.tabid = std::atoi(base_match[1].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a CHEC request tabid <" << request.tabid << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, raw_regex)) {
    request.str = line;
    request.type = Request::RequestType::raw;

    {
      // DB
      std::stringstream s;
      s << "Parsed a RAW request" << std::endl;
      db_log(s.str());
    }
  } else {
    {
      // DB
      std::stringstream s;
      s << "Cannot parse a request" << std::endl;
      db_log(s.str());
    }
    return false;
  }
  return true;
}

// parse request from frontend clients to tabservers
bool Parser::parse_secpri_request(const std::string &line,
                                  Request::SecPriRequest &request) {
  std::smatch base_match;
  if (std::regex_match(line, secpri_init_regex)) {
    request.type = Request::SecPriRequestType::init;

    {
      // DB
      std::stringstream s;
      s << "Parsed a INIT request from a secondary connection" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, secpri_hash_regex)) {
    request.type = Request::SecPriRequestType::hash;
    request.tabid = std::atoi(base_match[1].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a HASH request from a secondary connection" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, secpri_none_regex)) {
    request.type = Request::SecPriRequestType::none;
    request.tabid = std::atoi(base_match[1].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a NONE request from primary" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, secpri_both_regex)) {
    request.type = Request::SecPriRequestType::both;
    request.tabid = std::atoi(base_match[1].str().c_str());
    request.cp_file_size = std::atoi(base_match[2].str().c_str());
    request.lg_file_size = std::atoi(base_match[3].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a BOTH request from primary, cp size <"
        << request.cp_file_size << ">, lg size <" << request.lg_file_size << ">"
        << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, secpri_cp_regex)) {
    request.type = Request::SecPriRequestType::cp;
    request.tabid = std::atoi(base_match[1].str().c_str());
    request.cp_file_size = std::atoi(base_match[2].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a CP request from primary, size <" << request.cp_file_size
        << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, secpri_lg_regex)) {
    request.type = Request::SecPriRequestType::lg;
    request.tabid = std::atoi(base_match[1].str().c_str());
    request.lg_file_size = std::atoi(base_match[2].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a LG request from primary, size <" << request.lg_file_size
        << ">" << std::endl;
      db_log(s.str());
    }
  } else {
    {
      // DB
      std::stringstream s;
      s << "Cannot parse a request" << std::endl;
      db_log(s.str());
    }
    return false;
  }
  return true;
}

// parse request from master to tabserver
bool Parser::parse_matab_request(const std::string &line,
                                 Request::MaTabRequest &request) {
  std::smatch base_match;
  if (std::regex_match(line, base_match, matab_add_regex)) {
    request.type = Request::MaTabRequestType::add;
    request.tabid = std::atoi(base_match[1].str().c_str());

    {
      // DB
      std::stringstream s;
      s << "Parsed a ADD request tabid <" << request.tabid << ">" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, matab_kill_regex)) {
    request.type = Request::MaTabRequestType::kill;

    {
      // DB
      std::stringstream s;
      s << "Parsed a KILL request" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, matab_rest_regex)) {
    request.type = Request::MaTabRequestType::rest;

    {
      // DB
      std::stringstream s;
      s << "Parsed a REST request" << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, matab_init_regex)) {
    request.type = Request::MaTabRequestType::init;

    {
      // DB
      std::stringstream s;
      s << "Parsed a INIT request, from master" << std::endl;
      db_log(s.str());
    }
  } else {
    {
      // DB
      std::stringstream s;
      s << "Cannot parse a matab request" << std::endl;
      db_log(s.str());
    }
    return false;
  }
  return true;
}

// parse request from frontend to master
bool Parser::parse_fema_request(const std::string &line,
                                Request::FeMaRequest &request) {
  std::smatch base_match;
  if (std::regex_match(line, base_match, fema_look_regex)) {
    request.type = Request::FeMaRequestType::look;
    request.row = base_match[1].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a LOOK request row <" << request.row << ">" << std::endl;
      db_log(s.str());
    }
  } else {
    {
      // DB
      std::stringstream s;
      s << "Cannot parse a fema request" << std::endl;
      db_log(s.str());
    }
    return false;
  }
  return true;
}

// parse request from admin console to master
bool Parser::parse_amma_request(const std::string &line,
                                Request::AmMaRequest &request) {
  std::smatch base_match;
  if (std::regex_match(line, base_match, amma_kill_regex)) {
    request.type = Request::AmMaRequestType::kill;
    request.tabserver = base_match[1].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a KILL request tabserver <" << request.tabserver << ">"
        << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, amma_rest_regex)) {
    request.type = Request::AmMaRequestType::rest;
    request.tabserver = base_match[1].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a REST request tabserver <" << request.tabserver << ">"
        << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, amma_stat_regex)) {
    request.type = Request::AmMaRequestType::stat;

    {
      // DB
      std::stringstream s;
      s << "Parsed a STAT request" << std::endl;
      db_log(s.str());
    }
  } else {
    {
      // DB
      std::stringstream s;
      s << "Cannot parse a amma request" << std::endl;
      db_log(s.str());
    }
    return false;
  }
  return true;
}

// parse request from tabserver to master
bool Parser::parse_tabma_request(const std::string &line,
                                 Request::TabMaRequest &request) {
  std::smatch base_match;
  if (std::regex_match(line, base_match, tabma_join_regex)) {
    request.type = Request::TabMaRequestType::join;
    request.tabserver = base_match[1].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a JOIN request tabserver <" << request.tabserver << ">"
        << std::endl;
      db_log(s.str());
    }
  } else if (std::regex_match(line, base_match, tabma_stat_regex)) {
    request.type = Request::TabMaRequestType::stat;
    request.tabserver = base_match[1].str();

    {
      // DB
      std::stringstream s;
      s << "Parsed a STAT request tabserver <" << request.tabserver << ">"
        << std::endl;
      db_log(s.str());
    }
  } else {
    {
      // DB
      std::stringstream s;
      s << "Cannot parse a tabma request" << std::endl;
      db_log(s.str());
    }
    return false;
  }
  return true;
}
