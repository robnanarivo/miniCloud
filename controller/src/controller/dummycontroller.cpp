#include "dummycontroller.hpp"
#include "utils/utils.hpp"

void DummyController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    // compile a HTTP response for testing
    res.set_protocol("HTTP/1.1")
       .set_status_code(200)
       .set_header("Access-Control-Allow-Origin", "*")
       .set_header("Content-Type", "application/json")
       .set_body(DataChunk(">>> This is your original request >>>\n" + req.to_plain_string(false) + "<<< END of your request <<<"));
}
