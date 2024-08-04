#include "httpresponse.hpp"
#include "utils/utils.hpp"

HTTPResponse::HTTPResponse()
    : protocol("HTTP/1.1"),
      status_code(500),
      status_text("Server Errror: status text not set"),
      headers(),
      body() {}

std::string HTTPResponse::to_string(bool with_body) const {
    std::string res;

    res += to_red(protocol);
    res += ' ';
    res += to_green(std::to_string(status_code));
    res += ' ';
    res += to_red(status_text);
    res += '\n';
    for (const auto& pair : headers) {
        res += to_green(pair.first) + ": " + pair.second + '\n';
    }
    res += '\n';

    if (with_body) {
        for (size_t i = 0; i < body.size; i++) {
            res += body.data[i];
        }
    } else {
        res += "<body with length " + std::to_string(body.size) + " bytes>"; 
    }
    return res;
}

HTTPResponse& HTTPResponse::set_protocol(const std::string& proto) {
    protocol = proto;
    return *this;
}

HTTPResponse& HTTPResponse::set_status_code(unsigned short code) {
    status_code = code;
    if (status_chart.find(code) != status_chart.end()) {
        set_status_text(status_chart.at(code));
    }
    return *this;
}

HTTPResponse& HTTPResponse::set_status_text(const std::string& text) {
    status_text = text;
    return *this;
}

HTTPResponse& HTTPResponse::set_header(const std::string& key, const std::string& value) {
    headers[key] = value;
    return *this;
}

HTTPResponse& HTTPResponse::set_body(const DataChunk& chunk) {
    body = chunk;
    return *this;
}

HTTPResponse& HTTPResponse::set_body(DataChunk&& chunk) {
    body = std::move(chunk);
    return *this;
}

const std::unordered_map<unsigned short, std::string> HTTPResponse::status_chart = {
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {204, "No Content"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"}
};