#pragma once

#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>

#include <string>
#include <regex>
#include <ctime>

#include "utils/utils.hpp"
#include "storeclient/storeclient.hpp"

enum State {
  UNINIT,
  INIT,
  RCPT,
  DATA
};

class Mailbox {
  private:
    int comm_fd;
    std::string host_domain;
    std::string client_domain;
    std::string sender;
    std::vector<std::string> recipients;
    std::string content;
    State status;
    struct MailInfo &mail_info;
    StoreClient storeClient;

  public:
    Mailbox(int comm_fd, std::string host_domain, struct MailInfo &mail_info);
    
    // parse user input and write response to socket
    int parse_command(std::string command);

    // write response to socket
    bool send_client(std::string code, std::string to_send);

    // check if a given user exists
    bool exists_user(std::string user);

    // write email content to recipients' mailbox
    void write_email();

    // reset to initial state
    void reset();

    // tell client command is out of order
    void command_out_of_order();
};