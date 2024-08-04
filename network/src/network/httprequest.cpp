#include "httprequest.hpp"
#include "utils/utils.hpp"

std::string HTTPRequest::to_string(bool with_body) const {
    std::string res = HTTPRequest::method2str(method);
    res = to_red(res);
    res += ' ';
    res += to_green(target);
    res += ' ';
    res += to_red(protocol);
    res += '\n';

    // serialize headers
    for (const auto& pair : headers) {
        res += to_green(pair.first) + ": " + pair.second + '\n';
    }
    res += '\n';

    // serialize body
    if (with_body) {
        for (size_t i = 0; i < body.size; i++) {
            res += body.data[i];
        }
    } else {
        res += "<body with length " + std::to_string(body.size) + " bytes>"; 
    }

    return res;
}

std::string HTTPRequest::to_plain_string(bool with_body) const {
    std::string res = HTTPRequest::method2str(method);
    res += ' ';
    res += target;
    res += ' ';
    res += protocol;
    res += '\n';

    // serialize headers
    for (const auto& pair : headers) {
        res += pair.first + ": " + pair.second + '\n';
    }
    res += '\n';

    // serialize body
    if (with_body) {
        for (size_t i = 0; i < body.size; i++) {
            res += body.data[i];
        }
    } else {
        res += "<body with length " + std::to_string(body.size) + " bytes>"; 
    }

    return res;
}

std::string HTTPRequest::method2str(HTTP_METHOD method) {
    switch (method) {
    case HTTP_REQ_GET:
        return "GET";
        break;
    case HTTP_REQ_PUT:
        return "PUT";
        break;
    case HTTP_REQ_POST:
        return "POST";
        break;
    case HTTP_REQ_DELETE:
        return "DELETE";
        break;
    case HTTP_REQ_HEAD:
        return "HEAD";
        break;
    case HTTP_REQ_OPTIONS:
        return "OPTIONS";
        break;
    case HTTP_REQ_OTHER:
        return "OTHER";
        break;
    default:
        return "UNDEFINED";
        break;
    }
}