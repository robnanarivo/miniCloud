#include <iostream>

#include "controller/dispatchcontroller.hpp"
#include "httpserver/httpserver.hpp"
#include "utils/utils.hpp"

int main(int argc, char* argv[]) {
  // default ports
  Address http_external("localhost", 8080);
  Address dispatcher_external("localhost", 9000);

  // parse port
  int opt = 0;
  while ((opt = getopt(argc, argv, "h:l:")) != -1) {
    switch (opt) {
      case 'h':
        // parse port number
        http_external = Address(std::string(optarg));
        break;
      case 'l':
        dispatcher_external = Address(std::string(optarg));
        break;
      default:
        // unrecognized parameter
        std::cerr << "Usage: ./dispatcher [-h <hostname>:<port>] [-l "
                     "<hostname>:<port>]"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
  }

  HTTPServer server(http_external);
  log_info("Dispatcher HTTP listens on " + http_external.to_string());

  DispatchService dispatch_service(dispatcher_external.port);
  log_info("Dispatcher listens to heartbeat on " +
           dispatcher_external.to_string());
  DispatchController dispatch_controller(dispatch_service);

  server.use_default(dispatch_controller);
  server.set_log_mode(HTTP_SERVER_DEBUG);
  server.start();
}
