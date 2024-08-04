#pragma once

#include <string>
#include <vector>

class SocketReaderWriter {
 private:
  std::vector<char> buffer;
  bool fill_buffer();
  int has_line();

 public:
  int socket;
  SocketReaderWriter(int socket_);
  bool read_line(std::string &line);
  bool read_data(int size, std::vector<char> &data);
  bool write_line(const std::string &line);
  bool write_data(const std::vector<char> &data);
  void close_srw();
};