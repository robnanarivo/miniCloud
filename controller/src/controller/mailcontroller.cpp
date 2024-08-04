#include "mailcontroller.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "utils/utils.hpp"
using json = nlohmann::json;

MailController::MailController(MailService& mailService)
    : mailService_(mailService) {}

void MailController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    // check authentication
    std::string username;
    if (!userValidation(req, username)) {
        res.set_status_code(403);
        return;
    }

    try {
        if (req.method == HTTP_REQ_GET || req.method == HTTP_REQ_HEAD) {
            // get all mails for this user
            std::unordered_map<std::string, Email> mailList = mailService_.list_mails(username);
            // TODO: parse mailList to json
            std::vector<json> mailJsonList;
            for (const auto& pair : mailList) {
                mailJsonList.push_back(pair.second.to_json());
            }
            json body(mailJsonList);
            // set response
            res.set_status_code(200)
               .set_body(DataChunk(body.dump()));
        } else if (req.method == HTTP_REQ_POST) {
            // send a mail
            // get recipient email address, subject, and content from request body
            // auto body = json::parse(req.body.to_str());
            auto body = req.body.to_json();
            std::string recepient = body["address"].get<std::string>();
            std::string subject = body["subject"].get<std::string>();
            std::string content = body["message"].get<std::string>();

            bool sent = mailService_.send_mail(to_address(username), recepient, subject, content);
            if (!sent) {
                // fail to send
                res.set_status_code(404);
                return;
            }
            // mail successfully sent, set response
            res.set_status_code(201);
        } else if (req.method == HTTP_REQ_DELETE) {
            // delete a mail
            // get mail id from path
            std::string id = req.target.substr(1, req.target.length());

            bool deleted = mailService_.delete_mail(username, id);
            if (!deleted) {
                res.set_status_code(500);
                return;
            }
            res.set_status_code(200);
        } else if (req.method == HTTP_REQ_OPTIONS) {
            res.set_status_code(204);
            return;
        } else if (req.method == HTTP_REQ_OTHER) {
            res.set_status_code(501);
            Controller::add_req_as_body(req, res);
            return;
        }
    } catch (std::invalid_argument& e) {
        // Given path links to a folder
        res.set_status_code(400);
        log_err("MailController: " + std::string(e.what()));
        return;
    } catch (std::exception& e) {
        res.set_status_code(500);
        log_err("MailController: " + std::string(e.what()));
        return;
    }
}
