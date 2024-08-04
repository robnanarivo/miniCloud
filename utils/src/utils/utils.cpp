#include "utils.hpp"

#include <openssl/md5.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

std::string data2string(const ByteArray& data) {
  // const char* data_str = reinterpret_cast<const char*>(data.data());
  // if (data_str == nullptr) {
  //     return "";
  // }
  std::string content(data.begin(), data.end());
  return content;
}

ByteArray string2data(const std::string& str) {
  char char_array[str.length() + 1];
  strcpy(char_array, str.c_str());
  unsigned char* char_data = reinterpret_cast<unsigned char*>(char_array);
  ByteArray data;
  data.assign(char_data, char_data + str.length());
  return data;
}

std::vector<std::string> tokenize(const std::string& s, char delim) {
  std::vector<std::string> tokens;
  // init a string stream
  std::istringstream iss(s);
  std::string item;
  // parse the stream piece by piece
  while (std::getline(iss, item, delim)) {
    if (!item.empty()) {
      tokens.push_back(item);
    }
  }
  return tokens;
}

bool is_integer(const std::string& s) {
  // iterate through s
  for (const char& c : s) {
    if (!std::isdigit(c)) {
      return false;
    }
  }
  return true;
}

void lower(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

std::string to_lower(const std::string& s) {
  std::string res = s;
  lower(res);
  return res;
}

void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// trim from end (in place)
void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

// trim from both ends (in place)
void trim(std::string& s) {
  rtrim(s);
  ltrim(s);
}

std::string to_red(std::string s) {
  s = "\033[1;31m" + s + "\033[0m";
  return s;
}
std::string to_green(std::string s) {
  s = "\033[1;32m" + s + "\033[0m";
  return s;
}
std::string to_yellow(std::string s) {
  s = "\033[1;33m" + s + "\033[0m";
  return s;
}

std::string to_blue(std::string s) {
  s = "\033[1;34m" + s + "\033[0m";
  return s;
}

std::vector<std::string> split(std::string str, std::regex delims) {
  std::sregex_token_iterator first{str.begin(), str.end(), delims, -1}, last;
  std::vector<std::string> tokens{first, last};

  // remove empty strings
  tokens.erase(
      std::remove_if(tokens.begin(), tokens.end(),
                     [](std::string const& s) { return s.size() == 0; }),
      tokens.end());

  return tokens;
}

std::string getHash(std::string data) {
  unsigned char digestBuffer[MD5_DIGEST_LENGTH];

  // compute md5 hash
  MD5_CTX c;
  MD5_Init(&c);
  MD5_Update(&c, data.c_str(), data.size());
  MD5_Final(digestBuffer, &c);

  std::string result;
  for (std::size_t i = 0; i != 16; ++i) {
    result += "0123456789ABCDEF"[digestBuffer[i] / 16];
    result += "0123456789ABCDEF"[digestBuffer[i] % 16];
  }

  return result;
}

void log_info(const std::string& s) {
  std::cout << to_green("[INFO] ") << s << std::endl;
}

void log_err(const std::string& s) {
  std::cerr << to_red("[ERR] ") << s << std::endl;
}

void log_debug(const std::string& s) {
  std::cout << to_blue("[DEBUG] ") << s << std::endl;
}

#include <string>

size_t nthOccurrence(const std::string& str, const std::string& findMe,
                     int nth) {
  size_t pos = 0;
  int cnt = 0;
  while (cnt != nth) {
    pos += 1;
    pos = str.find(findMe, pos);
    if (pos == std::string::npos) return std::string::npos;
    cnt++;
  }
  return pos;
}

std::string to_address(const std::string& username) {
    return username + "@" + SERVER_DOMAIN;
}

bool begin_with(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

void trim_prefix(std::string& s, const std::string& prefix) {
    if (begin_with(s, prefix)) {
        s = s.substr(prefix.size(), std::string::npos);
    }
}

std::unordered_map<std::string, std::string> parse_target(const std::string& target, std::string& dest) {
    // param mapping
    std::unordered_map<std::string, std::string> res;

    // empty target
    if (target == "") {
        return res;
    }

    // split path and params
    size_t question_pos = target.find('?');
    dest = target.substr(0, question_pos);
    std::string param_str = target.substr(question_pos + 1);

    auto params_vec = tokenize(param_str, '&');
    for (const auto& param : params_vec) {
        auto param_pair = tokenize(param, '=');
        if (param_pair.size() == 2) {
            res.insert({param_pair.at(0), param_pair.at(1)});
        }
    }
    return res;
}