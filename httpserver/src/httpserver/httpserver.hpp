#pragma once

#include <mutex>

#include "controller/controller.hpp"
#include "network/tcpsocket.hpp"
#include "network/udpsocket.hpp"
#include "router.hpp"
#include "service/cookieservice.hpp"
#include "threadpool.hpp"

// HTTPService is singular

enum HTTP_SERVER_LOG_MODE {
  HTTP_SERVER_DEBUG,
  HTTP_SERVER_INFO,
  HTTP_SERVER_ERROR,
  HTTP_SERVER_SILENT,
};

class HTTPServer {
 private:
  // threadpool to track current threads
  ThreadPool thd_pool_;

  // embedded Router instance
  Router router_;

  // master socket
  TCPSocket tcpsocket_;

  // send udp port for telemetry to dispatcher
  UDPSocket udpsocket_;

  // server logging mode
  HTTP_SERVER_LOG_MODE log_mode_;

  // #connection being handled currently;
  size_t conn_num_;
  std::mutex lock_;

  // dispatcher to send heartbeat to
  Address dispatcher_;

  // The external HTTP port of this server
  Address external_address_;

  // cookie store service for cookie lookup
  CookieService* cookie_service_;

  // handle a child socket
  // this member function is delegated to a child thread
  void handle_connection(TCPSocket tcpsocket) noexcept;

  // heartbeat send daemon function
  void heartbeat_daemon_func() noexcept;

 public:
  // Ctor
  HTTPServer(const Address& external_address);

  // CCtor - deleted
  HTTPServer(const HTTPServer& other) = delete;

  // set logging mode
  void set_log_mode(HTTP_SERVER_LOG_MODE mode);

  // Dtor
  virtual ~HTTPServer() = default;

  // mount controller onto a endpoint
  void use(const std::string& endpoint, Controller& controller);

  // mount default controller
  void use_default(Controller& controller);

  // dispatch a thread to send heartbeats to a dispatcher
  void enable_heartbeat(const Address& dispatcher);

  // link to a cookie service
  // "username" field will be added to request headers after lookup
  void link_cookie_service(CookieService* cookie_service);

  // start accept connections and loop
  void start();
};
