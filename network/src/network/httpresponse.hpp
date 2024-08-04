#pragma once

#include <string>
#include <unordered_map>

#include "datachunk.hpp"

struct HTTPResponse {
  // protocol version "HTTP/1.1"
  std::string protocol;

  // 200, 404
  unsigned short status_code;

  // text description of status code "OK", "FORBIDDEN"
  std::string status_text;

  // header map
  std::unordered_map<std::string, std::string> headers;

  // chunk of data in body
  DataChunk body;

  // to readable string
  std::string to_string(bool with_body) const;

    // Ctor - default
    HTTPResponse();

    // dot-like setters
    // set protocol text
    HTTPResponse& set_protocol(const std::string& proto);

  // set status code, also sets status text if exists
  HTTPResponse& set_status_code(unsigned short code);

  // set status text
  HTTPResponse& set_status_text(const std::string& text);

  // set a header
  HTTPResponse& set_header(const std::string& key, const std::string& value);

  // set body
  HTTPResponse& set_body(const DataChunk& chunk);
  HTTPResponse& set_body(DataChunk&& chunk);

 private:
  static const std::unordered_map<unsigned short, std::string> status_chart;
};
