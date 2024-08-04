#pragma once

#include "controller.hpp"

class FallbackController : public Controller {
public:
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};
