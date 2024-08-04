#include <ctime>

#include "mailservice.hpp"

MailService::MailService(StorageService &storeServ)
    : storeServ_(storeServ), smtpClient_()
{
    try {
        // check if ".email" row exists
        storeServ_.list_folder(".email", "/");
    } catch (const std::exception& e) {
        // if not, create ".email" and root folder "/"
        storeServ_.initialize_row(".email");
    }
}

bool MailService::send_mail(std::string sender, std::string recipient, std::string subject, std::string content)
{
    std::vector<std::string> recipient_info = split(recipient, std::regex("@+"));
    if (recipient_info.size() != 2) {
        throw std::invalid_argument("recipient has no domain information");
    }
    std::string rcpt_user = recipient_info[0];
    std::string rcpt_domain = recipient_info[1];
    if (rcpt_domain == SERVER_DOMAIN) {
        std::string mail = "Subject: " + subject + "\n" + content;
        time_t now = time(0);
        char* dt = ctime(&now);
        mail = sender + "|" + dt + mail + "\n### EmAiL SePaRaToR ###\n";
        return send_mail_local(rcpt_user, mail);
    }
    
    return smtpClient_.send_mail(sender, recipient, subject, content);
}

bool MailService::send_mail_local(std::string user, std::string mail)
{
    ByteArray mailbox_data, new_mailbox_data;
    try {
        mailbox_data = storeServ_.read_file(".email", "/" + user);
    } catch (const std::exception& e) {
        // mailbox does not exist
        return false;
    }
    // use cput to avoid race condition
    do {
        mailbox_data = storeServ_.read_file(".email", "/" + user);
        std::string mailbox_content = data2string(mailbox_data);
        mailbox_content += mail;
        new_mailbox_data = string2data(mailbox_content);
    } while (!storeServ_.cond_write_file(".email", "/", user, new_mailbox_data, mailbox_data));
    return true;
}

std::unordered_map<std::string, Email> MailService::parse(std::string inbox_str)
{
    std::unordered_map<std::string, Email> inbox;
    for (std::string &mail_data : split(inbox_str, std::regex("(\n### EmAiL SePaRaToR ###\n)+"))) {
        std::string id = getHash(mail_data);
        int delim_index = mail_data.find_first_of("\n");
        std::string metadata = mail_data.substr(0, delim_index);
        std::string sender = metadata.substr(0, metadata.find_first_of("|"));
        std::string timestamp = metadata.substr(metadata.find_first_of("|") + 1);
        std::string content = mail_data.substr(delim_index + 1);
        Email mail(id, sender, timestamp, content);
        inbox[id] = mail;
    }
    return inbox;
}

std::string MailService::serialize(std::unordered_map<std::string, Email> &inbox)
{
    std::string mail_data;
    for (const std::pair<std::string, Email> &p : inbox) {
        const Email &mail = p.second;
        std::string data = mail.sender + "|" + mail.timestamp + "\n" + mail.content + "\n### EmAiL SePaRaToR ###\n";
        mail_data += data;
    }
    return mail_data;
}

std::unordered_map<std::string, Email> MailService::list_mails(std::string user)
{
    ByteArray data = storeServ_.read_file(".email", user);
    std::string inbox_str = data2string(data);
    std::unordered_map<std::string, Email> inbox = parse(inbox_str);
    return inbox;
}

bool MailService::delete_mail(std::string user, std::string id)
{
    ByteArray data = storeServ_.read_file(".email", user);
    std::string inbox_str = data2string(data);
    std::unordered_map<std::string, Email> inbox = parse(inbox_str);
    inbox.erase(id);
    ByteArray new_data = string2data(serialize(inbox));
    return storeServ_.cond_write_file(".email", "/", user, new_data, data);
}