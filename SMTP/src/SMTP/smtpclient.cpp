#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>


#include <stdexcept>
#include <limits>
#include <iostream>

#include "smtpclient.hpp"
#include "utils/utils.hpp"

#define N 4096

std::string SMTPClient::getMXDomain(std::string domain) {
    unsigned char nsbuf[N];
    char dispbuf[N];
    ns_rr rr;
    ns_msg msg;

    int l = res_query(domain.c_str(), ns_c_in, ns_t_mx, nsbuf, sizeof(nsbuf));
    if (l < 0) {
      perror(domain.c_str());
    }
    ns_initparse(nsbuf, l, &msg);
    int msg_len = l;
    l = ns_msg_count(msg, ns_s_an);

    int priority = std::numeric_limits<int>::max();
    std::string mx_domain = "";

    for (int i = 0; i < l; i++) {
        ns_parserr(&msg, ns_s_an, i, &rr);
        ns_sprintrr(&msg, &rr, NULL, NULL, dispbuf, sizeof(dispbuf));
        char exchange[NS_MAXDNAME];
        const u_char *rdata = ns_rr_rdata(rr);
        const uint16_t pri = ns_get16(rdata);
        int len = dn_expand(nsbuf, nsbuf + msg_len, rdata + 2, exchange, sizeof(exchange));

        if (pri < priority) {
            priority = pri;
            mx_domain = exchange;
        }
    }

    return mx_domain;
}

std::unique_ptr<TCPSocket> SMTPClient::getTCPSocket(std::string mx_domain)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    s = getaddrinfo(mx_domain.c_str(), nullptr, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return std::unique_ptr<TCPSocket>();
    }

    std::unique_ptr<TCPSocket> tcp = std::make_unique<TCPSocket>();
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        char ipstr[INET6_ADDRSTRLEN];
        struct sockaddr_in *addr_in = (struct sockaddr_in *)rp->ai_addr;
        void *addr = &(addr_in->sin_addr);
        inet_ntop(rp->ai_family, addr, ipstr, sizeof(ipstr));
        try {
            tcp->connect_to(ipstr, 25);
        } catch (std::runtime_error) {
            continue;
        }
        break;
    }

    // if connection is not established
    if (rp == NULL) {
        return std::unique_ptr<TCPSocket>();
    }

    return tcp;
}

bool SMTPClient::check_response(std::string response)
{
    // std::cout << response << std::endl;
    std::string res_code = split(response, std::regex("\\s+"))[0];
    if (res_code == "220" || res_code == "250" || res_code == "354") {
        return true;
    }
    return false;
}

bool SMTPClient::send_mail(std::string sender, std::string recipient, std::string subject, std::string content)
{
    std::vector<std::string> recipient_info = split(recipient, std::regex("@+"));
    if (recipient_info.size() != 2) {
        throw std::invalid_argument("recipient has no domain information");
    }

    // get MX domain
    std::string domain = recipient_info[1];
    std::string mx_domain = getMXDomain(domain);
    if (mx_domain.empty()) {
        throw std::invalid_argument("cannot find mx domain");
    }

    std::cout << mx_domain << std::endl;

    // get IP and port and create TCP connection
    std::unique_ptr<TCPSocket> tcp = getTCPSocket(mx_domain);
    if (!tcp) {
        return false;
    }

    // send mail
    std::string res_INIT = tcp->receive_line();
    std::cout << "res INIT: " << res_INIT << std::endl;

    // HELO
    std::vector<std::string> sender_info = split(sender, std::regex("@+"));
    if (sender_info.size() != 2) {
        throw std::invalid_argument("sender has no domain information");
    }
    std::string sender_domain = sender_info[1];
    tcp->send_str("HELO " + sender_domain);
    std::string res_HELO = tcp->receive_line();
    std::cout << "res HELO: " << res_HELO << std::endl;
    if (!check_response(res_HELO)) {
        return false;
    }
    
    // MAIL FROM
    tcp->send_str("MAIL FROM: <" + sender + ">");
    std::string res_MAIL = tcp->receive_line();
    std::cout << "res MAIL: " << res_MAIL << std::endl;
    if (!check_response(res_MAIL)) {
        return false;
    }
    
    // RCPT TO
    tcp->send_str("RCPT TO: <" + recipient + ">");
    std::string res_RCPT = tcp->receive_line();
    std::cout << "res RCPT: " << res_RCPT << std::endl;
    if (!check_response(res_RCPT)) {
        return false;
    }
    
    // DATA
    tcp->send_str("DATA");
    std::string res_DATA = tcp->receive_line();
    std::cout << "res DATA: " << res_DATA << std::endl;
    if (!check_response(res_DATA)) {
        return false;
    }
    
    // send content
    tcp->send_str("Subject: " + subject + "\n");
    tcp->send_str(content);
    tcp->send_str(".");
    std::string res_content = tcp->receive_line();
    std::cout << "res content: " << res_content << std::endl;
    tcp->send_str("quit");
    if (!check_response(res_content)) {
        return false;
    }
    return true;
}