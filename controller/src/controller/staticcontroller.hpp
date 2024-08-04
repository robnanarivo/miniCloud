#pragma once

#include "controller.hpp"
#include "service/staticservice.hpp"

class StaticController : public Controller {
private:
    StaticService& staticService_;
public:
    // constructor
    StaticController(StaticService& staticService);
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};