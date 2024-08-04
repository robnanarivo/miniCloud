#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cerrno>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "db_logger.h"
#include "master_node.h"
#include "parser.h"
#include "request.h"
#include "socket_reader_writer.h"
#include "task_queue.h"

// threads handling connections from frontend clients
void frontend_handler(MasterNode &master_node, TaskQueue &frontend_clients) {
  {
    // DB
    std::stringstream s;
    s << "New frontend_handler thread" << std::endl;
    db_log(s.str());
  }

  std::srand(std::time(nullptr));
  while (true) {
    // get a frontend connection
    int sock;
    if (!frontend_clients.deque(sock)) {
      continue;
    }

    {
      // DB
      std::stringstream s;
      s << "New frontend connection dequed: " << sock << std::endl;
      db_log(s.str());
    }

    // read and parse request
    SocketReaderWriter srw(sock);
    std::string line;
    srw.read_line(line);
    Request::FeMaRequest request;
    if (Parser::parse_fema_request(line, request)) {
      if (request.type == Request::FeMaRequestType::look) {
        // calculate tabid which this row belongs; having 6 tablets
        int tabid = std::hash<std::string>{}(request.row) % 6;

        {
          // DB
          std::stringstream s;
          s << "Tabid <" << tabid << "> for row <" << request.row << ">"
            << std::endl;
          db_log(s.str());
        }

        // find tabservers serving tablet id
        std::vector<std::string> &tabservers =
            master_node.tabid_to_tabservers.at(tabid);
        // randomly select 1 out of 3 tabservers in the cluster
        std::string tabserver = tabservers.at(std::rand() % 3);
        srw.write_line(tabserver + "\r\n");
      }
    } else {
      // cannot parse this request
      srw.write_line("-ERR invalid request\r\n");
    }
    srw.close_srw();
  }
}

// single thread handling connection from admin console
void admin_handler(MasterNode &master_node) {
  {
    // DB
    std::stringstream s;
    s << "New admin_handler thread" << std::endl;
    db_log(s.str());
  }

  // bind, listen, accept connections on admin_bind_addr
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int opt = 1;
  int status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                          sizeof(opt));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot set socket opt: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = bind(sock, (struct sockaddr *)&master_node.admin_bind_addr.sock_addr,
                sizeof(master_node.admin_bind_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot bind to addr: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = listen(sock, 5);
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot listen: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  while (true) {
    // new connection from admin console
    int conn = accept(sock, NULL, NULL);
    if (conn <= 0) {
      // DB
      std::stringstream s;
      s << "Cannot accept connection: " << strerror(errno) << std::endl;
      db_log(s.str());
      break;
    }

    {
      // DB
      std::stringstream s;
      s << "New admin connection: " << conn << std::endl;
      db_log(s.str());
    }

    // read and parse request
    SocketReaderWriter srw(conn);
    std::string line;
    srw.read_line(line);
    Request::AmMaRequest request;
    if (Parser::parse_amma_request(line, request)) {
      if (request.type == Request::AmMaRequestType::kill) {
        auto tabserver_itr = master_node.tabservers.find(request.tabserver);
        if (tabserver_itr != master_node.tabservers.end()) {
          if (std::time(nullptr) - tabserver_itr->second.last_heartbeat > 50) {
            srw.write_line("-ERR cannot kill tablet server; disconnected\r\n");
          } else if (tabserver_itr->second.is_killed) {
            srw.write_line(
                "-ERR cannot kill tablet server; already killed\r\n");
          } else {
            // forward kill request to tabserver
            SocketReaderWriter &tabserver_srw =
                master_node.tabserver_to_srw.at(request.tabserver);
            tabserver_itr->second.is_killed = true;
            tabserver_srw.write_line("KILL\r\n");
            srw.write_line("+OK KILL request forwarded to tablet serve\r\n");
          }
        } else {
          srw.write_line("-ERR cannot identify tablet server\r\n");
        }
      } else if (request.type == Request::AmMaRequestType::rest) {
        auto tabserver_itr = master_node.tabservers.find(request.tabserver);
        if (tabserver_itr != master_node.tabservers.end()) {
          if (!tabserver_itr->second.is_killed) {
            srw.write_line(
                "-ERR cannot restart tablet server; not yet killed\r\n");
          } else {
            // forward rest request to tabserver
            SocketReaderWriter &tabserver_srw =
                master_node.tabserver_to_srw.at(request.tabserver);
            tabserver_itr->second.is_killed = false;
            tabserver_srw.write_line("REST\r\n");
            srw.write_line("+OK REST request forwarded to tablet server\r\n");
          }
        } else {
          srw.write_line("-ERR cannot identify tablet server\r\n");
        }
      } else if (request.type == Request::AmMaRequestType::stat) {
        // send status of tabservers
        std::string response;
        std::time_t curr_time = std::time(nullptr);
        for (auto tabserver_itr = master_node.tabservers.begin();
             tabserver_itr != master_node.tabservers.end(); ++tabserver_itr) {
          if (curr_time - tabserver_itr->second.last_heartbeat > 50) {
            // tabserver is disconnected
            response.append(tabserver_itr->second.id + "=" + "0 ");
          } else {
            // tabserver is alive
            response.append(tabserver_itr->second.id + "=" + "1 ");
          }
        }
        response.append("\r\n");
        srw.write_line(response);
      }
    } else {
      // cannot parse this request
      srw.write_line("-ERR invalid request\r\n");
    }
    srw.close_srw();
  }

  close(sock);
}

// single thread handling connections from tabservers
void tabserver_handler(MasterNode &master_node) {
  {
    // DB
    std::stringstream s;
    s << "New tabserver_handler thread" << std::endl;
    db_log(s.str());
  }

  // bind, listen, accept connections on tabserver_bind_addr
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int opt = 1;
  int status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                          sizeof(opt));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot set socket opt: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status =
      bind(sock, (struct sockaddr *)&master_node.tabserver_bind_addr.sock_addr,
           sizeof(master_node.tabserver_bind_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot bind to addr: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = listen(sock, 50);
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot listen: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  while (true) {
    // new connection from a tabserver
    int conn = accept(sock, NULL, NULL);
    if (conn <= 0) {
      // DB
      std::stringstream s;
      s << "Cannot accept connection: " << strerror(errno) << std::endl;
      db_log(s.str());
      break;
    }

    {
      // DB
      std::stringstream s;
      s << "New tabserver connection: " << conn << std::endl;
      db_log(s.str());
    }

    // read and parser request
    SocketReaderWriter srw(conn);
    std::string line;
    srw.read_line(line);
    Request::TabMaRequest request;

    if (Parser::parse_tabma_request(line, request)) {
      if (request.type == Request::TabMaRequestType::join) {
        // JOIN request informs that the tabserver comes online
        auto tabserver_itr = master_node.tabservers.find(request.tabserver);
        if (tabserver_itr != master_node.tabservers.end()) {
          // send back the tablet names and ids that it should serve
          std::vector<int> &tabids = tabserver_itr->second.tabids;
          for (auto tabid_itr = tabids.begin(); tabid_itr != tabids.end();
               ++tabid_itr) {
            srw.write_line("ADD " + std::to_string(*tabid_itr) + "\r\n");
          }
          srw.write_line("INIT\r\n");
          // record heartbeat time
          tabserver_itr->second.last_heartbeat = std::time(nullptr);
          // save this connection for sending future admin control requests
          master_node.tabserver_to_srw.insert_or_assign(request.tabserver,
                                                        std::move(srw));
        } else {
          // cannot find this tabserver in config file
          srw.write_line("-ERR cannot identify tablet server\r\n");
          srw.close_srw();
        }
      } else if (request.type == Request::TabMaRequestType::stat) {
        // STAT request informs that the tabserver is alive
        auto tabserver_itr = master_node.tabservers.find(request.tabserver);
        if (tabserver_itr != master_node.tabservers.end()) {
          tabserver_itr->second.last_heartbeat = std::time(nullptr);
        }
        srw.close_srw();
      }
    } else {
      // cannot parse this request
      srw.write_line("-ERR invalid request\r\n");
      srw.close_srw();
    }
  }

  close(sock);
}

int main(int argc, char *argv[]) {
  // macros
  int thread_pool_size = 15;

  // parse command line args: config_file, node_index
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [config_file]\r\n", argv[0]);
    return -1;
  }
  std::string config_file = argv[1];

  // create data structures
  MasterNode master_node;
  TaskQueue frontend_connections;

  // parse configuration file
  Parser::parse_master_config(config_file, master_node);

  {
    // DB: Master node meta info
    std::stringstream s;
    s << "Master node info: "
      << "frontend bind <" << master_node.frontend_bind_addr.sock_addr_str
      << "> admin bind <" << master_node.admin_bind_addr.sock_addr_str
      << "> tabserver bind <" << master_node.tabserver_bind_addr.sock_addr_str
      << "> number of tabservers <" << master_node.tabservers.size()
      << "> number of tablets <" << master_node.tabid_to_tabservers.size()
      << ">" << std::endl;
    db_log(s.str());
  }

  std::vector<std::thread> thread_pool;

  // start tabserver handler thread
  thread_pool.push_back(std::thread(tabserver_handler, std::ref(master_node)));

  // start admin handler thread
  thread_pool.push_back(std::thread(admin_handler, std::ref(master_node)));

  // start frontend handler thread
  for (int i = 0; i < thread_pool_size; ++i) {
    thread_pool.push_back(std::thread(frontend_handler, std::ref(master_node),
                                      std::ref(frontend_connections)));
  }

  {
    // DB
    std::stringstream s;
    s << "All threads created: " << thread_pool.size() << std::endl;
    db_log(s.str());
  }

  // accept frontend connections
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int opt = 1;
  int status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                          sizeof(opt));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot set socket opt: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status =
      bind(sock, (struct sockaddr *)&master_node.frontend_bind_addr.sock_addr,
           sizeof(master_node.frontend_bind_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot bind to addr: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = listen(sock, 50);
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot listen: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  while (true) {
    int new_conn = accept(sock, NULL, NULL);
    if (new_conn <= 0) {
      // DB
      std::stringstream s;
      s << "Cannot accept connection: " << strerror(errno) << std::endl;
      db_log(s.str());
      break;
    }
    frontend_connections.enqueue(new_conn);
  }

  // clean up
  close(sock);

  for (std::thread &thread : thread_pool) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  return 0;
}
