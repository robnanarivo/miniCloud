#pragma once

#include <map>
#include <string>

#include "controller/controller.hpp"
#include "network/httprequest.hpp"
#include "network/httpresponse.hpp"

// comparator
struct cmpByStringLength {
    bool operator()(const std::string& a, const std::string& b) const {
        if (a.length() != b.length()) {
            return a.length() > b.length();
        }
        return a < b;
    }
};

// Router is responsible for routing a request to a controller
class Router {
private:
    // route table
    std::map<std::string, Controller*, cmpByStringLength> route_table_;

    // fallback controller
    Controller* fallback_controller_;

public:

    // register controller to an endpoint
    void add_route(const std::string& endpoint, Controller* controller);

    // register fallback controller
    void add_fallback_controller(Controller* fallback_controller);

    // route a request, like process_request in controller
    void route_request(HTTPRequest& req, HTTPResponse& res);
};

