#include "httpserver.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "utils/utils.hpp"

HTTPServer::HTTPServer(const Address& external_address)
    : thd_pool_(),
      router_(),
      tcpsocket_(),
      udpsocket_(),
      log_mode_(HTTP_SERVER_INFO),
      conn_num_(0),
      lock_(),
      dispatcher_("", 0),
      external_address_(external_address),
      cookie_service_(nullptr)
{
    tcpsocket_.bind_and_listen(external_address.port, true);
}

void HTTPServer::set_log_mode(HTTP_SERVER_LOG_MODE mode) { log_mode_ = mode; }

void HTTPServer::handle_connection(TCPSocket tcpsocket) noexcept
{
    // update connection count
    lock_.lock();
    conn_num_++;
    lock_.unlock();

    auto &client_addr = tcpsocket.get_client_info();

    if (log_mode_ == HTTP_SERVER_DEBUG)
    {
        log_debug("HTTPServer: New Conn. from " + client_addr.hostname + ":" +
                  std::to_string(client_addr.port) +
                  " (fd: " + std::to_string(tcpsocket.get_socket_fd()) + ")");
    }
    while (true)
    {
        try
        {
            // read request
            HTTPRequest req = tcpsocket.receive_http_req();
            if (log_mode_ == HTTP_SERVER_DEBUG)
            {
                log_debug("HTTPServer: New Req. from " + client_addr.hostname + ":" +
                          std::to_string(client_addr.port) +
                          " (fd: " + std::to_string(tcpsocket.get_socket_fd()) + "):");
                std::cout << req.to_string(false) << std::endl;
            }

            // instantiate response
            HTTPResponse res;

            // middleware - check cookie and add "username" field in req headers
            // if such cookie does not exist, no "username" field is added to headers
            if (req.headers.find("cookie") != req.headers.end() &&
                cookie_service_ != nullptr)
            {
                // has cookie header, must parse cookie
                auto cookies =
                    CookieService::parse_cookie_header(req.headers.at("cookie"));
                if (cookies.find("auth_token") != cookies.end())
                {
                    // has token
                    auto token = cookies.at("auth_token");
                    auto username = cookie_service_->get_user(token);
                    if (username.has_value())
                    {
                        // valid token
                        log_debug("username mapped: " + username.value());
                        req.headers["username"] = username.value();
                    }
                }
            }

            // delegate request to controller
            router_.route_request(req, res);

            // post-processing
            Controller::add_alive_headers(req, res);
            Controller::add_cors_headers(req, res);
            Controller::set_content_length(res);
            Controller::trim_body_for_head(req, res);

            // log result
            if (log_mode_ == HTTP_SERVER_DEBUG)
            {
                log_debug("HTTPServer: Res. to " + client_addr.hostname + ":" +
                          std::to_string(client_addr.port) + " (fd: " +
                          std::to_string(tcpsocket.get_socket_fd()) + "):");
                std::cout << res.to_string(false) << std::endl;
            }

            // send response
            tcpsocket.send_http_res(res);
            
            if (log_mode_ <= HTTP_SERVER_INFO)
            {
                log_info(to_red(HTTPRequest::method2str(req.method)) + ' ' +
                         req.target + ' ' +
                         to_green(std::to_string(res.status_code)) + ' ' +
                         to_blue(res.status_text));
            }
            if (res.headers.find("Connection") == res.headers.end() ||
                to_lower(res.headers.at("Connection")) != "keep-alive")
            {
                if (log_mode_ == HTTP_SERVER_DEBUG)
                {
                    log_debug("fd: " + std::to_string(tcpsocket.get_socket_fd()) + " no keep-alive");
                }
                break;
            }
        }
        catch (const std::runtime_error& e)
        {
            // network connection broken
            if (log_mode_ == HTTP_SERVER_DEBUG)
            {
                log_debug("fd: " + std::to_string(tcpsocket.get_socket_fd()) + " broken");
            }
            break;
        }
        catch (const std::exception& e) {
            // unknown exception
            log_err("HTTPServer: unknown exception: \"" + std::string(e.what()) + "\"");
            break;
        }
    }
    if (log_mode_ == HTTP_SERVER_DEBUG)
    {
        log_debug("HTTPServer: Terminating conn. from " + std::to_string(tcpsocket.get_socket_fd()));
    }

    // decrement connection count
    lock_.lock();
    conn_num_--;
    lock_.unlock();
}

void HTTPServer::heartbeat_daemon_func() noexcept
{
    while (true)
    {
        try {
            // sleep for 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
            lock_.lock();
            udpsocket_.send_str(external_address_.to_string() + ',' +
                                    std::to_string(conn_num_),
                                dispatcher_);
            lock_.unlock();
        } catch (const std::exception& e) {
            log_err("HTTPServer: heartbeat daemon: \"" + std::string(e.what()) + "\"");
            continue;
        }
    }
}

void HTTPServer::use(const std::string &endpoint, Controller &controller)
{
    router_.add_route(endpoint, &controller);
}

void HTTPServer::use_default(Controller &controller)
{
    router_.add_fallback_controller(&controller);
}

void HTTPServer::enable_heartbeat(const Address &dispatcher)
{
    dispatcher_.hostname = dispatcher.hostname;
    dispatcher_.port = dispatcher.port;
    std::thread heartbeat_daemon(&HTTPServer::heartbeat_daemon_func, this);
    heartbeat_daemon.detach();
}

void HTTPServer::link_cookie_service(CookieService *cookie_service)
{
    cookie_service_ = cookie_service;
}

void HTTPServer::start()
{
    while (true)
    {
        TCPSocket conn = tcpsocket_.accept_connection();
        std::thread thd(&HTTPServer::handle_connection, this, std::move(conn));
        thd.detach();
    }
}
