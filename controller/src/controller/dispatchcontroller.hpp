#pragma once

#include "controller.hpp"
#include "service/dispatchservice.hpp"

#define RANDOM_NODE false

class DispatchController : public Controller {
private:
    DispatchService& dispatch_service_;
public:
    DispatchController(DispatchService& dispatch_service);
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};
