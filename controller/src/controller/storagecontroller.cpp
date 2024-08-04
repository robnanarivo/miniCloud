#include "storagecontroller.hpp"
#include "utils/utils.hpp"
#include "network/staticfile.hpp"


StorageController::StorageController(StorageService& storeServ)
    : storeServ_(storeServ) {}

void StorageController::process_http_request(const HTTPRequest& req, HTTPResponse& res) noexcept {
    // check authentication
    std::string username;
    if (!userValidation(req, username)) {
        res.set_status_code(403);
        return;
    }

    try {
        if (req.method == HTTP_REQ_GET || req.method == HTTP_REQ_HEAD) {
            std::string path;
            std::unordered_map<std::string, std::string> params = parse_target(req.target, path);
            if (params.find("type") == params.end()) {
                res.set_status_code(400);
                return;
            }

            if (params["type"] == "file") {
                ByteArray file_data = storeServ_.read_file(username, path);
                res.set_body(DataChunk(file_data))
                   .set_status_code(200)
                   .set_header("Content-Type", StaticFile::parse_content_type(path));
                return;
            } else if (params["type"] == "folder") {
                std::vector<FolderElement> folder_files = storeServ_.list_folder(username, path);
                std::vector<json> filelist;
                for (const auto& elem : folder_files) {
                    json file;
                    file["filename"] = elem.filename;
                    file["hash"] = elem.hash;
                    filelist.push_back(file);
                }
                res.set_body(DataChunk(json(filelist).dump()))
                    .set_status_code(200);
                return;
            }
        } else if (req.method == HTTP_REQ_PUT) {
            json body = req.body.to_json();
            std::string type = body["type"].get<std::string>();
            std::string new_name = body["newName"].get<std::string>();
            std::string from = body["from"].get<std::string>();
            std::string to = body["to"].get<std::string>();
            std::string method = body["method"].get<std::string>();
            log_debug("type: " + type);
            log_debug("newName: " + new_name);
            log_debug("from: " + from);
            log_debug("to: " + to);
            log_debug("method: " + method);
            if (type == "file") {
                if (storeServ_.move_file(username, from, to, new_name)) {
                    res.set_status_code(200);
                } else {
                    res.set_status_code(400);
                }
                return;
            } else if (type == "folder") {
                if (method == "move") {
                    if(storeServ_.move_folder(username, from, to, new_name)) {
                        res.set_status_code(200);
                    } else {
                        res.set_status_code(400);
                    }
                    return;
                } else if (method == "rename") {
                    if (storeServ_.rename_folder(username, from, new_name)) {
                        res.set_status_code(200);
                    } else {
                        res.set_status_code(400);
                    }
                    return;
                }
            }
        } else if (req.method == HTTP_REQ_POST) {
            log_debug("POST new file");
            // create a file or folder
            ByteArray file_data = req.body.to_vec();
            log_debug(std::to_string(file_data.size()));
            std::string path;
            std::unordered_map<std::string, std::string> params = parse_target(req.target, path);
            // for (const auto& param : params) {
            //     log_debug("param found: " + param.first + " -> " + param.second);
            // }
            if (params.find("type") == params.end()) {
                res.set_status_code(400);
                return;
            }
            if (params.find("filename") == params.end()) {
                res.set_status_code(400);
                return;
            }

            if (params["type"] == "file") {
                // path must be parent folder
                if (storeServ_.write_file(username, path, params["filename"], file_data)) {
                    res.set_status_code(201);
                } else {
                    res.set_status_code(400);
                }
                return;
            } else if (params["type"] == "folder") {
                if (storeServ_.create_folder(username, path, params["filename"])) {
                    res.set_status_code(201);
                } else {
                    res.set_status_code(400);
                }
                return;
            }
        } else if (req.method == HTTP_REQ_DELETE) {
            if (storeServ_.delete_file(username, req.target)) {
                res.set_status_code(200);
            } else {
                res.set_status_code(400);
            }
            return;
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
        log_err("StorageController: " + std::string(e.what()));
        return;
    } catch (std::exception& e) {
        res.set_status_code(500);
        log_err("StorageController: " + std::string(e.what()));
        return;
    }
}
