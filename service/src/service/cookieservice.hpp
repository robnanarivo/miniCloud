#pragma once

#include <unordered_map>
#include <mutex>
#include <optional>

class CookieService {
private:
    // cookie ->username
    std::unordered_map<std::string, std::string> cookie_map_;
    // lock for the map
    std::mutex lock_;
public:
    void add_cookie_record(const std::string& user, const std::string& cookie);
    std::optional<std::string> get_user(const std::string& cookie);

    static std::unordered_map<std::string, std::string> parse_cookie_header(const std::string& header_value);
};
