#include "router.hpp"

#include "utils/utils.hpp"

void Router::add_route(const std::string& endpoint, Controller* controller) {
  route_table_[endpoint] = controller;
}

void Router::add_fallback_controller(Controller* fallback_controller) {
    fallback_controller_ = fallback_controller;
}

void Router::route_request(HTTPRequest& req, HTTPResponse& res) {
    // check each route
    for (const auto& pair : route_table_) {
        if (begin_with(req.target, pair.first)) {
            // find a match - best match ensured!
            trim_prefix(req.target, pair.first);
            log_debug("Router: " + pair.first + " -> " + req.target);
            pair.second->process_http_request(req, res);
            return;
        }
    }

    // no match, try to route to fallback controller
    if (fallback_controller_ == nullptr) {
        throw std::runtime_error("Router: fallback controller not registered");
    }
    fallback_controller_->process_http_request(req, res);
}
