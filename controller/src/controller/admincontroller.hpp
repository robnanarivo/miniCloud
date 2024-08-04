#pragma once

#include "controller.hpp"
#include "service/adminservice.hpp"

class AdminController : public Controller {
 private:
  AdminService& adminService_;

 public:
  // constructor
  AdminController(AdminService& adminService);
  virtual void process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept override;
};