#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "address.h"
#include "parser.h"
#include "socket_reader_writer.h"

int main(int argc, char* argv[]) {
  // parse addr
  Address server_addr = Parser::parse_addr(argv[1]);

  // parse row, col
  std::string row(argv[2]);
  std::string col(argv[3]);

  // get file content
  std::ifstream fstrm(argv[4]);
  std::istreambuf_iterator<char> fstart(fstrm), fend;
  std::vector<char> content(fstart, fend);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  connect(sock, (const struct sockaddr*)&server_addr.sock_addr,
          sizeof(server_addr.sock_addr));
  SocketReaderWriter srw(sock);

  srw.write_line("PUT " + row + " " + col + " " +
                 std::to_string(content.size()) + "\r\n");
  srw.write_data(content);

  std::string response;
  srw.read_line(response);

  if (response.substr(0, 1) == "+") {
    std::cout << "succeeded" << std::endl;
  } else {
    std::cout << "failed" << std::endl;
  }

  return 0;
}
