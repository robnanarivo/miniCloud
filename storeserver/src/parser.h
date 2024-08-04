#pragma once

#include <netinet/ip.h>

#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "address.h"
#include "master_node.h"
#include "request.h"
#include "tablet_node.h"

namespace Parser {

// parse ip address and port from a string
Address parse_addr(std::string addr);

// parser sockaddr into string representation
std::string parse_sockaddr(const sockaddr_in &src);

// parse tabserver configuration file
void parse_config(const std::string &config_file, const int &index,
                  TabletNode &tablet_node);

// parse master configuration file
bool parse_master_config(const std::string &config_file,
                         MasterNode &master_node);

// regex for requests from frontend clients to tabservers
bool parse_request(const std::string &line, Request::Request &request);

// regex for requests from secondaries to primary, and back
bool parse_secpri_request(const std::string &line,
                          Request::SecPriRequest &request);

// regex for requests from master to tabservers
bool parse_matab_request(const std::string &line,
                         Request::MaTabRequest &request);

// regex for requests from frontend clients to master
// LOOK: look up a tablet server serving this row
bool parse_fema_request(const std::string &line, Request::FeMaRequest &request);

// regex for requests from admin console to master
// KILL: kill the tabserver
// REST: restart the tabserver
// STAT: status of all tabservers
bool parse_amma_request(const std::string &line, Request::AmMaRequest &request);

// regex for requests from tabserver to master
// JOIN: tabserver comes online
// STAT: tabserver is alive
bool parse_tabma_request(const std::string &line,
                         Request::TabMaRequest &request);

}  // namespace Parser