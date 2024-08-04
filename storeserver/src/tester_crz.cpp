#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <thread>
#include <vector>

#include "address.h"
#include "parser.h"

int main(int argc, char* argv[]) {
  // parse addr
  Address server_addr = Parser::parse_addr(argv[1]);

  // get file content
  std::ifstream fstrm(argv[2]);
  std::istreambuf_iterator<char> fstart(fstrm), fend;
  std::vector<char> content(fstart, fend);

  int start = std::atoi(argv[3]);
  int num_copies = std::atoi(argv[4]);

  for (int i = start; i < start + num_copies; ++i) {
    for (int j = start; j < start + num_copies; ++j) {
      int sock = socket(AF_INET, SOCK_STREAM, 0);
      connect(sock, (const struct sockaddr*)&server_addr.sock_addr,
              sizeof(server_addr.sock_addr));
      std::string cmd("PUT " + std::to_string(i) + " " + std::to_string(j) +
                      " " + std::to_string(content.size()) + "\r\n");
      write(sock, cmd.c_str(), cmd.size());
      write(sock, &content[0], content.size());
      close(sock);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  return 0;
}