#include "mailbox.hpp"

Mailbox::Mailbox(int comm_fd, std::string host_domain, struct MailInfo &mail_info)
    : comm_fd(comm_fd),
    host_domain(host_domain), 
    client_domain(),
    sender(),
    recipients(),
    content(),
    status(UNINIT),
    mail_info(mail_info),
    storeClient {NetworkInfo(mail_info.master_ip, mail_info.master_port)}
    {
        send_client("220", host_domain + " Service Ready");
    }

int Mailbox::parse_command(std::string command) {
    // HELO command
    if (std::regex_match(command, std::regex("^helo\\s+\\S+\\s*$", std::regex::icase))) {
        if (status != UNINIT && status != INIT) {
            command_out_of_order();
        } else {
            client_domain = split(command, std::regex("\\s+"))[1];
            status = INIT;
            send_client("250", host_domain);
        }
        return 0;
    }

    // MAIL FROM command
    if (std::regex_match(command, std::regex("^mail\\s+from\\s*:\\s*<[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+>\\s*$", std::regex::icase))) {
        if (sender != "") {
            send_client("554", "Transaction failed; revert to initial state");
            reset();
        } else if (status != INIT) {
            command_out_of_order();
        } else {
            sender = split(command, std::regex("([\\s:<>]+)"))[2];
            send_client("250", "OK");
            status = RCPT;
        }
        return 0;
    }

    // RCPT TO command
    if (std::regex_match(command, std::regex("^rcpt\\s+to\\s*:\\s*<[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+>\\s*$", std::regex::icase))) {
        if (status != RCPT) {
            command_out_of_order();
        } else {
            std::string rcpt_email = split(command, std::regex("([\\s:<>]+)"))[2];
            std::vector<std::string> email_addr = split(rcpt_email, std::regex("@+"));
            if (email_addr[1] != SERVER_DOMAIN || !exists_user(email_addr[0])) {
                send_client("550", "Mailbox not available");
            } else {
                recipients.push_back(email_addr[0]);
                send_client("250", "OK");
            }
        }
        return 0;
    }

    // DATA command
    if (std::regex_match(command, std::regex("^data\\s*$", std::regex::icase)) ) {
        if (status != RCPT) {
            command_out_of_order();
        } else {
            status = DATA;
            send_client("354", "Please input email body");
        }
        return 0;
    }

    // RSET command
    if (std::regex_match(command, std::regex("^rset\\s*\\S*\\s*$", std::regex::icase))) {
        if (status == UNINIT) {
            command_out_of_order();
        } else {
            reset();
            send_client("250", "OK");
        }
        return 0;
    }

    // QUIT command
    if (std::regex_match(command, std::regex("^quit\\s*\\S*\\s*$", std::regex::icase))) {
        if (status == UNINIT) {
            command_out_of_order();
        } else {
            send_client("221", host_domain + " Service closing transmission channel");
        }
        return -1;
    }

    // NOOP command
    if (std::regex_match(command, std::regex("^noop\\s*\\S*\\s*$", std::regex::icase))) {
        if (status == UNINIT) {
            command_out_of_order();
        } else {
            send_client("250", "OK");
        }
        return 0;
    }

    // received text
    if (status == DATA) {
        if (command == ".") {
            // write email
            write_email();
            reset();
        } else {
            command += "\r\n";
            content += command;
        }
    } else {
        // command not recognized
        send_client("500", "Command not recognized");
    }

    return 0;
}

bool Mailbox::send_client(std::string code, std::string to_send) {
    std::string message = code + " " + to_send;
    // each message must end with <CR><LF>
    message += "\r\n";

    int sent = 0;
    int len = message.length();
    while (sent < len) {
        int n = write(comm_fd, &message.c_str()[sent], len - sent);
        if (n < 0) {
            return false;
        }
        sent += n;
    }
    return true;
}

bool Mailbox::exists_user(std::string user) {
    if (mail_info.user2box.find(user) == mail_info.user2box.end()) {
        return false;
    }
    return true;
}

void Mailbox::write_email() {
    time_t now = time(0);
    char* dt = ctime(&now);
    content = sender + "|" + dt + content + "\n### EmAiL SePaRaToR ###\n";
    bool success_once = false;

    for (std::string recipient : recipients) {
        bool delivered = true;
        std::string box_hash = mail_info.user2box[recipient];
        int lock_id = mail_info.box2lock[box_hash];
        pthread_mutex_t &mail_lock = mail_info.mail_locks[lock_id];

        pthread_mutex_lock(&mail_lock);
        
        TabletKey key(".email", box_hash);
        ByteArray mailbox_data, new_mailbox_data;
        // use cput to avoid race condition
        do {
            mailbox_data = storeClient.get(key);
            // if mailbox does not exist, break
            if (mailbox_data.size() == 0) {
                delivered = false;
                break;
            }
            std::string mailbox_content = data2string(mailbox_data);
            mailbox_content += content;
            new_mailbox_data = string2data(mailbox_content);
        } while (!storeClient.cput(key, mailbox_data, new_mailbox_data));

        // release file lock
        pthread_mutex_unlock(&mail_lock);

        if (delivered) {
            success_once = true;
        }
    }

    if (success_once) {
        send_client("250", "OK");
    } else {
        send_client("451", "Requested action aborted: error in processing");
    }
}

void Mailbox::reset() {
    sender = "";
    recipients = std::vector<std::string>();
    content = "";
    status = INIT;
}

void Mailbox::command_out_of_order() {
    send_client("503", "Bad sequence of commands");
}