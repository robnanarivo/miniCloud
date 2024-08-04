#include <iostream>

#include "SMTP/smtpclient.hpp"

int main() {
    SMTPClient smtp_c;
    bool res = smtp_c.send_mail("quack@penncloud.com", "wyq@seas.upenn.edu", "I am a duck", "Quak quack qwak!");
    if (!res) {
        std::cout << "fail" << std::endl;
    } else {
        std::cout << "success" << std::endl;
    }
}