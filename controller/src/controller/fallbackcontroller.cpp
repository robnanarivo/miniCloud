#include "fallbackcontroller.hpp"
#include "utils/utils.hpp"

void FallbackController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    // compile a HTTP response for fall back, usually 404
    res.set_status_code(404);
}
