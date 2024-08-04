#pragma once

#include <string>
#include <unordered_map>

#include "datachunk.hpp"

enum HTTP_METHOD {
  HTTP_REQ_GET,
  HTTP_REQ_POST,
  HTTP_REQ_PUT,
  HTTP_REQ_DELETE,
  HTTP_REQ_HEAD,
  HTTP_REQ_OPTIONS,
  HTTP_REQ_OTHER,
};

struct HTTPRequest {
public:
    HTTP_METHOD method;
    std::string target;
    std::string protocol;

    // header keys are in lower case!
    std::unordered_map<std::string, std::string> headers;

    // body represented in form of datachunk
    DataChunk body;

    // serialize HTTP request for debug purpose
    std::string to_string(bool with_body) const;
    std::string to_plain_string(bool with_body) const;

    static std::string method2str(HTTP_METHOD method);
};
