#pragma once

#include "controller.hpp"
#include "service/userservice.hpp"
#include "service/cookieservice.hpp"

class UserController : public Controller {
private:
    UserService& userServ_;
    CookieService& cookieServ_;
public:
    // constructor
    UserController(UserService& userService, CookieService& cookieService);
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};
