#pragma once

#include "controller.hpp"
#include "service/mailservice.hpp"

class MailController : public Controller {
private:
    MailService& mailService_;
public:
    // constructor
    MailController(MailService& mailService);
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};