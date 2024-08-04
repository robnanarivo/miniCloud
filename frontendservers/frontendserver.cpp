#include <iostream>

#include "controller/admincontroller.hpp"
#include "controller/dummycontroller.hpp"
#include "controller/mailcontroller.hpp"
#include "controller/storagecontroller.hpp"
#include "controller/usercontroller.hpp"
#include "controller/fallbackcontroller.hpp"
#include "httpserver/httpserver.hpp"
#include "service/adminservice.hpp"
#include "service/cookieservice.hpp"
#include "service/dispatchservice.hpp"
#include "service/mailservice.hpp"
#include "service/storageservice.hpp"
#include "service/userservice.hpp"
#include "storeclient/storeclient.hpp"
#include "utils/utils.hpp"
#include "service/staticservice.hpp"
#include "controller/staticcontroller.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    // default args
    Address http_external("localhost", 8080);
    Address dispatcher("127.0.0.1", 9000);
    bool enable_heartbeat = false;
    std::string react_build_dir("./react_build");

    // parse port
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:d:b:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            // parse port number
            http_external = Address(std::string(optarg));
            break;
        case 'd':
            dispatcher = Address(std::string(optarg));
            enable_heartbeat = true;
            break;
        case 'b':
            react_build_dir = std::string(optarg);
            if (react_build_dir.back() != '/')
            {
                react_build_dir += '/';
            }
            break;
        default:
            // unrecognized parameter
            std::cerr << "Usage: ./frontendserver [-h <hostname>:<port>] [-d <hostname>:<port>] [-b ./react/build]"
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // declare server
    HTTPServer server(http_external);
    log_info("Frontend server running on " + http_external.to_string());

    // Admin stuff
    StoreClient admin_client(NetworkInfo("127.0.0.1", 8002));
    AdminService admin_service(admin_client, dispatcher);
    AdminController admin_con(admin_service);

    // services
    StaticService static_service(react_build_dir);
    log_info("StaticService linked to dir: " + react_build_dir);
    StoreClient store_client(NetworkInfo("127.0.0.1", 8001));
    StorageService store_service(store_client);
    MailService mail_service(store_service);
    UserService user_service(store_service);
    CookieService cookie_service;

    // declare controllers
    FallbackController fallback_controller;
    StaticController static_controller(static_service);
    StorageController store_con(store_service);
    UserController user_con(user_service, cookie_service);
    MailController mail_con(mail_service);

    // declare server and associate controllers to endpoints
    server.set_log_mode(HTTP_SERVER_DEBUG);
    server.link_cookie_service(&cookie_service);
    server.use("/api/mail", mail_con);
    server.use("/api/storage", store_con);
    server.use("/api/user", user_con);
    server.use("/api/admin", admin_con);
    server.use("/", static_controller);
    server.use_default(fallback_controller);

    // enable heartbeat to dispatcher
    if (enable_heartbeat) {
        server.enable_heartbeat(dispatcher);
        log_info("Frontend server sending heartbeat to " + dispatcher.to_string());
    }

    // start inf loop
    server.start();
}
