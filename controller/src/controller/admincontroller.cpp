#include "admincontroller.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "utils/utils.hpp"
using json = nlohmann::json;

AdminController::AdminController(AdminService& adminService)
    : adminService_(adminService) {}

void AdminController::process_http_request(const HTTPRequest& req,
                                           HTTPResponse& res) noexcept {
  try {
    if (req.method == HTTP_REQ_GET || req.method == HTTP_REQ_HEAD) {
      if (req.target == "/nodes") {
        // get nodes and status
        std::unordered_map<std::string, bool> servs_stats =
            adminService_.list_backend_server();
        // send back request
        std::vector<json> pairs;
        for (auto serv_stat = servs_stats.begin();
             serv_stat != servs_stats.end(); ++serv_stat) {
          json j;
          j["addr"] = serv_stat->first;
          j["stat"] = serv_stat->second;
          pairs.push_back(j);
        }
        // send back request
        json body(pairs);
        res.set_status_code(200).set_body(DataChunk(body.dump()));
      } else if (req.target == "/frontendnodes") {
        // get frontend nodes and loads
        std::unordered_map<Address, size_t> servs_stats =
            adminService_.list_frontend_load();
        // send back request
        std::vector<json> pairs;
        for (auto serv_stat = servs_stats.begin();
             serv_stat != servs_stats.end(); ++serv_stat) {
          json j;
          j["addr"] = serv_stat->first.to_string();
          j["load"] = std::to_string(serv_stat->second);
          pairs.push_back(j);
        }
        // send back request
        json body(pairs);
        res.set_status_code(200).set_body(DataChunk(body.dump()));
      } else {
        res.set_status_code(400);
      }
    } else if (req.method == HTTP_REQ_PUT) {
      if (req.target == "/node") {
        // parse node address
        json body = req.body.to_json();
        std::string addr = body["addr"].get<std::string>();
        // get row_col pairs
        std::vector<TabletKey> rows_cols =
            adminService_.list_server_files(addr);
        // send back request
        std::vector<json> pairs;
        for (auto row_col = rows_cols.begin(); row_col != rows_cols.end();
             ++row_col) {
          json j;
          j["row"] = row_col->row;
          j["col"] = row_col->col;
          pairs.push_back(j);
        }
        json result_body(pairs);
        res.set_status_code(200).set_body(DataChunk(result_body.dump()));
      } else if (req.target == "/raw") {
        // parse node address
        json body = req.body.to_json();
        std::string addr = body["addr"].get<std::string>();
        std::string row = body["row"].get<std::string>();
        std::string col = body["col"].get<std::string>();
        ByteArray raw_data = adminService_.get_raw(addr, row, col);
        res.set_status_code(200).set_body(DataChunk(raw_data));
      } else if (req.target == "/kill") {
        // parse node address
        json body = req.body.to_json();
        std::string addr = body["addr"].get<std::string>();
        if (adminService_.kill_server(addr)) {
          res.set_status_code(200);
        } else {
          res.set_status_code(400);
        }
      } else if (req.target == "/rest") {
        // parse node address
        json body = req.body.to_json();
        std::string addr = body["addr"].get<std::string>();
        if (adminService_.reboot_server(addr)) {
          res.set_status_code(200);
        } else {
          res.set_status_code(400);
        }
      } else {
        res.set_status_code(400);
      }
    } else if (req.method == HTTP_REQ_OPTIONS) {
        res.set_status_code(204);
      return;
    } else if (req.method == HTTP_REQ_OTHER) {
      res.set_status_code(501);
      Controller::add_req_as_body(req, res);
      return;
    }
  } catch (std::invalid_argument& e) {
    res.set_status_code(400);
    log_err("AdminController: " + std::string(e.what()));
    return;
  } catch (std::exception& e) {
    res.set_status_code(500);
    log_err("AdminController: " + std::string(e.what()));
    return;
  }
}
