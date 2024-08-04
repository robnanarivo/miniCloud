#include <string>
#include <memory>

#include "network/tcpsocket.hpp"

class SMTPClient
{
private:
    std::string getMXDomain(std::string domain);
    std::unique_ptr<TCPSocket> getTCPSocket(std::string mx_domain);
    bool check_response(std::string response);

public:
    SMTPClient() = default;
    bool send_mail(std::string sender, std::string recipient, std::string subject, std::string content);
};