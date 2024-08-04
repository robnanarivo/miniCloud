#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <unordered_map>

#include "db_logger.h"
#include "parser.h"
#include "request_queue.h"
#include "socket_reader_writer.h"
#include "tablet_node.h"
#include "tablet_worker.h"
#include "tablet_worker_primary.h"
#include "tablet_worker_secondary.h"
#include "task_queue.h"

int main(int argc, char *argv[]) {
  // macro variables
  int thread_pool_size = 15;
  int is_restart = false;

  // parse command line option
  int opt;
  while ((opt = getopt(argc, argv, "r")) != -1) {
    switch (opt) {
      case 'r':
        is_restart = true;
        break;
      default:
        std::cerr << "Usage: tablet_server [config_file] [node_index] [-r]"
                  << std::endl;
        return -1;
    }
  }

  // parse command line args: config_file, node_index
  if (optind != argc - 2) {
    std::cerr << "Usage: tablet_server [config_file] [node_index] [-r]"
              << std::endl;
    return -1;
  }
  std::string config_file = argv[optind];
  int index = std::atoi(argv[++optind]);

  // create data structure
  TabletNode tablet_node;

  // parse config file, populate addresses
  Parser::parse_config(config_file, index, tablet_node);
  // for debug log info
  tablet_node.index = index;

  {
    // DB: Master node meta info
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Tablet node info: "
      << "primary <" << std::boolalpha << tablet_node.is_primary
      << "> frontend bind <" << tablet_node.frontend_bind_addr.sock_addr_str
      << "> master addr <" << tablet_node.master_addr.sock_addr_str << ">"
      << std::endl;
    db_log(s.str());
  }

  // start thread for sending heartbeat msg
  std::thread control_thread(TabletWorker::master_handler,
                             std::ref(tablet_node), thread_pool_size,
                             is_restart);

  control_thread.join();

  return 0;
}
