#include "dispatchcontroller.hpp"


DispatchController::DispatchController(DispatchService& dispatch_service)
    : dispatch_service_(dispatch_service) {}

void DispatchController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    try {
        auto frontend_server = dispatch_service_.get_frontend_server(RANDOM_NODE);
        if (frontend_server.has_value()) {
            res.set_status_code(301)
               .set_header("Location", "http://" + frontend_server.value().to_string() + req.target);
        } else {
            res.set_status_code(503);
        }
    } catch (const std::exception& e) {
        res.set_status_code(500);
    }
}
