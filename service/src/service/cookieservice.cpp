#include "cookieservice.hpp"
#include "utils/utils.hpp"

void CookieService::add_cookie_record(const std::string& user, const std::string& cookie) {
    lock_.lock();
    cookie_map_[cookie] = user;
    lock_.unlock();
}

std::optional<std::string> CookieService::get_user(const std::string& cookie) {
    lock_.lock();
    // for (const auto& record : cookie_map_) {
    //     log_debug("Cookie record: " + record.first + " -> " + record.second);
    // }
    std::optional<std::string> res{};
    if (cookie_map_.find(cookie) != cookie_map_.end()) {
        log_debug("Found username: " + cookie_map_.at(cookie));
        res = {cookie_map_.at(cookie)};
    }
    lock_.unlock();
    return res;
}

std::unordered_map<std::string, std::string> CookieService::parse_cookie_header(const std::string& header_value) {
    std::unordered_map<std::string, std::string> res;
    auto cookies = tokenize(header_value, ';');
    for (auto cookie : cookies) {
        trim(cookie);
        auto cookietokens = tokenize(cookie, '=');
        if (cookietokens.size() == 2) {
            res[cookietokens.at(0)] = cookietokens.at(1);
        }
    }
    return res;
}
