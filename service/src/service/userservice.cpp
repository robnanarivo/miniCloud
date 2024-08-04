#include "userservice.hpp"

UserService::UserService(StorageService &storeServ)
    : storeServ_(storeServ) 
{
    try {
        // check if ".email" row exists
        storeServ_.list_folder(".user", "/");
    } catch (const std::exception& e) {
        // if not, create ".email" and root folder "/"
        storeServ_.initialize_row(".user");
    }
}

bool UserService::login(std::string user, std::string password)
{
    std::string correct_pass;
    try {
        correct_pass = data2string(storeServ_.read_file(".user", "/" + user));
    } catch (const std::exception& e) {
        return false;
    }

    return (password == correct_pass);
}

bool UserService::sign_up(std::string user, std::string password)
{
    try {
        // check if user exists
        storeServ_.read_file(".user", "/" + user);
    } catch (const std::exception& e) {
        // user does not exist -> sign up is allowed
        bool init_success = storeServ_.initialize_row(user);
        bool mail_init_success = storeServ_.write_file(".email", "/", user, string2data(""));
        bool user_init_success = storeServ_.write_file(".user", "/", user, string2data(password));
        return init_success && mail_init_success && user_init_success;
    }
    return false;
}

bool UserService::change_password(std::string user, std::string new_password)
{
    return storeServ_.write_file(".user", "/", user, string2data(new_password));
}
