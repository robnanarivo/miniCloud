#include "controller.hpp"
#include "utils/utils.hpp"

bool Controller::userValidation(const HTTPRequest& req, std::string& username) {
    // for (const auto& header : req.headers) {
    //     log_debug("header pair: " + header.first + " -> " + header.second);
    // }
    if (req.headers.find("username") != req.headers.end()) {
        username = req.headers.at("username");
        return true;
    }
    return false;
}

void Controller::add_alive_headers(const HTTPRequest& req, HTTPResponse& res) {
    if (req.headers.find("connection") != req.headers.end() && to_lower(req.headers.at("connection")) == "keep-alive") {
        res.set_header("Connection", "Keep-Alive")
           .set_header("Keep-Alive", "timeout=20, max=999");
    }
}

void Controller::add_cors_headers(const HTTPRequest& req, HTTPResponse& res) {
    res.set_header("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS, DELETE, HEAD")
       .set_header("Access-Control-Allow-Credentials", "true")
       .set_header("Access-Control-Allow-Headers", "*")
       .set_header("Access-Control-Max-Age", "86400")
       .set_header("Access-Control-Expose-Headers", "Set-Cookie");
    if (req.headers.find("origin") != req.headers.end()) {
        res.set_header("Access-Control-Allow-Origin", req.headers.at("origin"));
    } else {
        res.set_header("Access-Control-Allow-Origin", "*");
    }
}

void Controller::add_req_as_body(const HTTPRequest& req, HTTPResponse& res) {
    res.set_body(req.to_plain_string(false));
}

void Controller::set_content_length(HTTPResponse& res) {
    res.set_header("Content-Length", std::to_string(res.body.size));
}

void Controller::trim_body_for_head(const HTTPRequest& req, HTTPResponse& res) {
    if (req.method == HTTP_REQ_HEAD) {
        res.set_body(DataChunk(0));
    }
}