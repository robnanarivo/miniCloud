#pragma once

#include "storageservice.hpp"

class UserService
{
private:
    StorageService &storeServ_;

public:
    UserService(StorageService &storeServ);
    bool login(std::string user, std::string password);
    bool sign_up(std::string user, std::string password);
    // only call change_password after the user has already logged in
    bool change_password(std::string user, std::string new_password);
};
