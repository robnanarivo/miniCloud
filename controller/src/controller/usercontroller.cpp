#include "usercontroller.hpp"
#include "utils/utils.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

UserController::UserController(UserService& userService, CookieService& cookieService)
    : userServ_(userService), cookieServ_(cookieService) {}

void UserController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    try {
        if (req.method == HTTP_REQ_POST) {
            // get username and password from body
            json user = req.body.to_json();
            std::string username = user["username"];
            std::string password = user["password"];
            // sign up and login
            if (req.target == "/signup") {
                if (!userServ_.sign_up(username, password)) {
                    res.set_status_code(400);
                    return;
                }
                res.set_status_code(201);
            } else if (req.target == "/login") {
                if (!userServ_.login(username, password)) {
                    // password and username does not match
                    res.set_status_code(401);
                    return;
                }
                // login success, add username to cookie
                std::string token = to_lower(getHash(username));
                cookieServ_.add_cookie_record(username, token);
                res.set_header("Set-Cookie", "auth_token=" + token + "; Path=/api");
                res.set_status_code(200);
            }
        } else if (req.method == HTTP_REQ_PUT) {
            // get username and password from body
            json user = req.body.to_json();
            std::string username = user["username"];
            std::string password = user["password"];
            // check authentication
            if (!userValidation(req, username)) {
                // not valid to change password
                res.set_status_code(403);
                return;
            }
            // change password
            if (!userServ_.change_password(username, password)) {
                res.set_status_code(400);
                return;
            }
            res.set_status_code(200);
        } else if (req.method == HTTP_REQ_OPTIONS) {
            res.set_status_code(204);
            return;
        } else if (req.method == HTTP_REQ_OTHER) {
            res.set_status_code(501);
            Controller::add_req_as_body(req, res);
            return;
        }
    } catch (std::invalid_argument& e) {
        res.set_status_code(400);
        log_err("UserController: " + std::string(e.what()));
        return;
    } catch (std::exception& e) {
        res.set_status_code(500);
        log_err("UserController: " + std::string(e.what()));
        return;
    }
}