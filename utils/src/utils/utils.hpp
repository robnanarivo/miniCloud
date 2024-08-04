#pragma once
#include <pthread.h>
#include <unistd.h>

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

// legacy code for SMTP and POP3 server -- START

// struct for synchronizing writing to email
struct MailInfo {
  std::string master_ip;
  unsigned short master_port;
  std::unique_ptr<pthread_mutex_t[]> mail_locks;
  std::unordered_map<std::string, int> box2lock;
  std::unordered_map<std::string, std::string> user2box;
};

// legacy code for SMTP and POP3 server -- END

#define ByteArray std::vector<unsigned char>

#define SERVER_DOMAIN "penncloud.com"

std::string data2string(const ByteArray& data);

ByteArray string2data(const std::string& str);

struct TabletKey {
  std::string row;
  std::string col;

  TabletKey(std::string row, std::string col) : row(row), col(col) {}
};

std::vector<std::string> split(std::string str, std::regex delims);

std::string getHash(std::string data);

// split s into multiple parts with delim char
std::vector<std::string> tokenize(const std::string& s, char delim);

// check is every char in s is a digit
bool is_integer(const std::string& s);

// convert string into lower case
void lower(std::string& s);
std::string to_lower(const std::string& s);

// trim white spaces
// trim from start (in place)
void ltrim(std::string& s);

// trim from end (in place)
void rtrim(std::string& s);

// trim from both ends (in place)
void trim(std::string& s);

// text coloring
std::string to_red(std::string s);
std::string to_green(std::string s);
std::string to_yellow(std::string s);
std::string to_blue(std::string s);

// logging to console
void log_info(const std::string& s);
void log_err(const std::string& s);
void log_debug(const std::string& s);

// find n-th occurrence of a string in a long string
size_t nthOccurrence(const std::string& str, const std::string& findMe,
                     int nth);

// change username to email address
std::string to_address(const std::string& username);

// find if string begins with another string
bool begin_with(const std::string& s, const std::string& prefix);

// parse the remaining target URI
std::unordered_map<std::string, std::string> parse_target(
    const std::string& target, std::string& dest);

// trim prefix
void trim_prefix(std::string& s, const std::string& prefix);
