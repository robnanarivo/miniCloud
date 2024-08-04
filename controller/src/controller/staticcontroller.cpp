#include "staticcontroller.hpp"
#include "utils/utils.hpp"

StaticController::StaticController(StaticService& staticService) 
    : staticService_(staticService) {}
void StaticController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    std::string filename = req.target;
    if (filename == "") {
        filename = "index.html";
    }
    try {
        if (req.method == HTTP_REQ_GET || req.method == HTTP_REQ_HEAD) {
            StaticFile file = staticService_.read_file(filename);
            res.set_header("Content-type", file.type)
               .set_status_code(200)
               .set_body(file.data);
            return;
        } else {
            res.set_status_code(501);
            Controller::add_req_as_body(req, res);
            return;
        }
    } catch (std::runtime_error& e) {
        res.set_status_code(404);
        log_err("StaticController: " + std::string(e.what()));
        return;
    } catch (std::invalid_argument& e) {
        res.set_status_code(400);
        log_err("StaticController: " + std::string(e.what()));
        return;
    } catch (std::exception& e) {
        res.set_status_code(500);
        log_err("StaticController: " + std::string(e.what()));
        return;
    }
}
