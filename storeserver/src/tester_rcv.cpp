#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
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
  std::ofstream fstrm(argv[4]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  connect(sock, (const struct sockaddr*)&server_addr.sock_addr,
          sizeof(server_addr.sock_addr));
  SocketReaderWriter srw(sock);

  srw.write_line("GET " + row + " " + col + "\r\n");

  std::string response;
  srw.read_line(response);

  if (response.substr(0, 1) == "+") {
    int data_size = std::atoi(response.substr(4, response.size()).c_str());
    std::vector<char> data_content;
    srw.read_data(data_size, data_content);
    fstrm.write(&data_content[0], data_content.size());
    fstrm.close();
  } else {
    std::cout << "error" << std::endl;
  }

  srw.close_srw();

  return 0;
}