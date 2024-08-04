#pragma once

#include "controller.hpp"
#include "service/storageservice.hpp"

class StorageController : public Controller {
private:
    StorageService& storeServ_;
public:
    StorageController(StorageService& storeServ);
    virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};
