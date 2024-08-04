#include "socket_reader_writer.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <sstream>
#include <thread>

#include "db_logger.h"

SocketReaderWriter::SocketReaderWriter(int socket_) : socket(socket_) {}

// Find line terminator '\n', return index of '\n' char
// Otherwise return -1
int SocketReaderWriter::has_line() {
  for (int i = 0; i < buffer.size(); ++i) {
    if (buffer[i] == '\n') {
      return i;
    }
  }
  return -1;
}

// Fill buffer up to buf_size bytes at a time
bool SocketReaderWriter::fill_buffer() {
  // Reserve enough space for buffer
  int curr_size = buffer.size();
  buffer.resize(curr_size + 1024);
  // Read into buffer and set to correct size
  int size_read = recv(socket, &buffer[curr_size], 1024, 0);
  if (size_read == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // // DB
      // std::stringstream s;
      // s << "recv() timed out: " << strerror(errno) << std::endl;
      // db_log(s.str());
    } else {
      // DB
      std::stringstream s;
      s << "recv() error: " << strerror(errno) << std::endl;
      db_log(s.str());
    }
    buffer.resize(curr_size);
    return false;
  }
  buffer.resize(curr_size + size_read);
  return true;
}

// Return a line, including '\n'
bool SocketReaderWriter::read_line(std::string &line) {
  int index;
  while ((index = has_line()) == -1) {
    if (!fill_buffer()) {
      return false;
    }
  }

  line = std::string(buffer.begin(), buffer.begin() + index + 1);
  buffer.erase(buffer.begin(), buffer.begin() + index + 1);

  {
    // DB
    std::stringstream s;
    s << "Read line: " << line;
    db_log(s.str());
  }

  return true;
}

// Return size bytes of data from buffer
bool SocketReaderWriter::read_data(int size, std::vector<char> &data) {
  if (size == 0) {
    data = std::vector<char>();
    return true;
  }

  while (buffer.size() < size) {
    if (!fill_buffer()) {
      return false;
    }
  }

  data = std::vector<char>(buffer.begin(), buffer.begin() + size);
  buffer.erase(buffer.begin(), buffer.begin() + size);

  {
    // DB
    std::stringstream s;
    s << "Read data " << data.size() << " bytes" << std::endl;
    db_log(s.str());
  }

  return true;
}

// Write line to socket
bool SocketReaderWriter::write_line(const std::string &line) {
  int write_size = send(socket, line.c_str(), line.size(), MSG_NOSIGNAL);

  if (write_size == -1) {
    // DB
    std::stringstream s;
    s << "Cannot write to socket: " << strerror(errno) << std::endl;
    db_log(s.str());
    return false;
  } else {
    // DB
    std::stringstream s;
    s << "Wrote line: " << line;
    db_log(s.str());
  }

  return true;
}

// Write data to socket
bool SocketReaderWriter::write_data(const std::vector<char> &data) {
  int write_size = send(socket, &data[0], data.size(), MSG_NOSIGNAL);

  if (write_size == -1) {
    // DB
    std::stringstream s;
    s << "Cannot write to socket: " << strerror(errno) << std::endl;
    db_log(s.str());
    return false;
  } else {
    // DB
    std::stringstream s;
    s << "Wrote data: " << data.size() << " bytes" << std::endl;
    db_log(s.str());
  }

  return true;
}

// Close socket
void SocketReaderWriter::close_srw() { close(socket); }