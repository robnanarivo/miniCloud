#pragma once

#include "network/httprequest.hpp"
#include "network/httpresponse.hpp"

class Controller {
public:
    bool userValidation(const HTTPRequest& req, std::string& username);
    virtual ~Controller() = default;
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept = 0;

    static void add_alive_headers(const HTTPRequest& req, HTTPResponse& res);
    static void add_cors_headers(const HTTPRequest& req, HTTPResponse& res);
    static void add_req_as_body(const HTTPRequest& req, HTTPResponse& res);
    static void set_content_length(HTTPResponse& res);
    static void trim_body_for_head(const HTTPRequest& req, HTTPResponse& res);
};
