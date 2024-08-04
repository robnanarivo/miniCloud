#pragma once

#include "storeclient/storeclient.hpp"
#include "SMTP/smtpclient.hpp"
#include "storageservice.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct Email
{
    std::string id;
    std::string sender;
    std::string timestamp;
    std::string content;

    Email()
        : id(), sender(), timestamp(), content() {}
    
    Email(std::string id, std::string sender, std::string timestamp, std::string content)
        : id(id), sender(sender), timestamp(timestamp), content(content) {}

    // convertion between Email and json
    Email(json j)
        : Email(j["id"].get<std::string>(), j["sender"].get<std::string>(), 
                j["timestamp"].get<std::string>(), j["content"].get<std::string>()) {}

    json to_json() const {
        json j;
        j["id"] = id;
        j["sender"] = sender;
        j["timestamp"] = timestamp;
        j["content"] = content;
        return j;
    }
};

// we assume that there is only one session for each mailbox (the same user cannot login twice)
class MailService
{
private:
    StorageService storeServ_;
    SMTPClient smtpClient_;
    bool send_mail_local(std::string user, std::string mail);
    std::unordered_map<std::string, Email> parse(std::string inbox_str);
    std::string serialize(std::unordered_map<std::string, Email> &inbox);

public:
    MailService(StorageService &storeServ);
    bool send_mail(std::string sender, std::string recipient, std::string subject, std::string content);
    std::unordered_map<std::string, Email> list_mails(std::string user);
    bool delete_mail(std::string user, std::string id);
};
