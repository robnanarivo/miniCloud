#include "tablet_worker_secondary.h"

#include <cerrno>
#include <sstream>

#include "db_logger.h"
#include "parser.h"

// forward request to parimary node
std::string TabletWorkerSecondary::forward_to_primary(
    TabletNode &tablet_node, Request::Request request) {
  // open a new connection to primary node
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int status = connect(
      sock, (const struct sockaddr *)&tablet_node.primary_addr.sock_addr,
      sizeof(tablet_node.primary_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot connect: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  SocketReaderWriter srw(sock);

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Forward to primary: " << request.str;
    db_log(s.str());
  }

  // send request and data
  if (request.type == Request::RequestType::put) {
    srw.write_line(request.str);
    srw.write_data(request.v1);
  } else if (request.type == Request::RequestType::cput) {
    srw.write_line(request.str);
    srw.write_data(request.v1);
    srw.write_data(request.v2);
  } else if (request.type == Request::RequestType::dele) {
    srw.write_line(request.str);
  } else if (request.type == Request::RequestType::chec) {
    srw.write_line(request.str);
  }

  std::string response;
  srw.read_line(response);

  srw.close_srw();

  return response;
}

// secondary node interfacing with frontend clients
// blocked on wait()
void TabletWorkerSecondary::secondary_frontend_handler(TabletNode &tablet_node,
                                                       TaskQueue &task_queue) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New secondary_frontend_handler thread" << std::endl;
    db_log(s.str());
  }

  while (!tablet_node.is_disabled) {
    // get a frontend connection from task queue
    int sock;
    if (!task_queue.deque(sock)) {
      // {
      //   // DB
      //   std::stringstream s;
      //   s << "T" << tablet_node.index << " ";
      //   s << "Task queue timed out" << std::endl;
      //   db_log(s.str());
      // }
      continue;
    }

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "New frontend connection dequed: " << sock << std::endl;
      db_log(s.str());
    }

    // create buffered socket reader
    SocketReaderWriter srw(sock);

    // read and parse request
    std::string line;
    srw.read_line(line);
    Request::Request request;

    if (Parser::parse_request(line, request)) {
      // calculate tabid for the row
      if (request.type == Request::RequestType::raw) {
        std::multimap<std::string, std::string> rows_cols;
        bool op_result = tablet_node.get_raw(rows_cols);

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "RAW <" << std::boolalpha << op_result << ">, #pairs <"
            << rows_cols.size() << ">" << std::endl;
          db_log(s.str());
        }

        srw.write_line("+OK " + std::to_string(rows_cols.size()) + "\r\n");

        for (auto row_col = rows_cols.begin(); row_col != rows_cols.end();
             ++row_col) {
          srw.write_line(row_col->first + " " + row_col->second + "\r\n");
        }
      } else {
        int row_tabid = std::hash<std::string>{}(request.row) % 6;

        if (tablet_node.tablets.count(row_tabid) == 0) {
          // tabid is not served in this cluster
          srw.write_line("-ERR cannot find data; incorrect cluster\r\n");
        } else {
          // tabid is served in this cluster

          if (request.type == Request::RequestType::get) {
            // process GET request locally

            int curr_tabid = tablet_node.curr_tablet->id;

            if (row_tabid != curr_tabid) {
              // requested tablet is not in memory
              // need to checkpoint current tablet and load requested tablet

              {
                // DB
                std::stringstream s;
                s << "T" << tablet_node.index << " ";
                s << "Current tabid <" << curr_tabid << ">, requested tabid <"
                  << row_tabid << ">" << std::endl;
                db_log(s.str());
              }

              // checkpoint current tablet
              std::pair<bool, int> cp_result =
                  tablet_node.checkpoint_tablet(curr_tabid);

              if (cp_result.first) {
                // checkpoint succeeded

                // send checkpoint sync request
                Request::Request chec_request;
                chec_request.str =
                    "CHEC " + std::to_string(curr_tabid) + "\r\n";
                chec_request.type = Request::RequestType::chec;
                chec_request.tabid = curr_tabid;
                forward_to_primary(tablet_node, std::move(chec_request));

                // load requested tablet
                std::pair<bool, int> ld_result =
                    tablet_node.load_tablet(row_tabid);

                if (ld_result.first) {
                  {
                    // DB
                    std::stringstream s;
                    s << "T" << tablet_node.index << " ";
                    s << "Tablet <" << row_tabid << "> size <"
                      << tablet_node.tablet.size()
                      << "> after loading, prior to replay" << std::endl;
                    db_log(s.str());
                  }

                  // requested tablet successfully loaded into memory
                  tablet_node.replay_log();

                  {
                    // DB
                    std::stringstream s;
                    s << "T" << tablet_node.index << " ";
                    s << "Tablet <" << row_tabid << "> size <"
                      << tablet_node.tablet.size() << "> after replay"
                      << std::endl;
                    db_log(s.str());
                  }

                  {
                    // DB
                    std::stringstream s;
                    s << "T" << tablet_node.index << " ";
                    s << "Checkpoint <" << curr_tabid
                      << "> succeeded, loading <" << row_tabid
                      << "> succeeded, log replayed" << std::endl;
                    db_log(s.str());
                  }
                } else {
                  // loading did not occur
                  // because tablet has already been loaded
                  {
                    // DB
                    std::stringstream s;
                    s << "T" << tablet_node.index << " ";
                    s << "Checkpoint <" << curr_tabid
                      << "> succeeded, but loading <" << row_tabid
                      << "> did not occur because it has already been loaded"
                      << std::endl;
                    db_log(s.str());
                  }
                }
              } else {
                // checkpoint did not succeed
                // because tablet has already been moved out of memory
                {
                  // DB
                  std::stringstream s;
                  s << "T" << tablet_node.index << " ";
                  s << "Checkpoint did not occur because <" << curr_tabid
                    << "> has already been moved out of memory" << std::endl;
                  db_log(s.str());
                }
              }
            }

            // now, the requested tablet has been loaded
            // perform the GET request
            std::vector<char> val;
            bool gt_result = tablet_node.get(request, val);
            if (gt_result) {
              srw.write_line("+OK " + std::to_string(val.size()) + "\r\n");
              srw.write_data(val);
            } else {
              srw.write_line("-ERR cannot find data; nonexistent\r\n");
            }
          } else if (request.type == Request::RequestType::put) {
            srw.read_data(request.v1_size, request.v1);

            // forward put request to primary
            std::string response = TabletWorkerSecondary::forward_to_primary(
                tablet_node, std::move(request));
            srw.write_line(response);
          } else if (request.type == Request::RequestType::cput) {
            srw.read_data(request.v1_size, request.v1);
            srw.read_data(request.v2_size, request.v2);

            // forward cput request to primary
            std::string response = TabletWorkerSecondary::forward_to_primary(
                tablet_node, std::move(request));
            srw.write_line(response);
          } else if (request.type == Request::RequestType::dele) {
            // forward dele request to primary
            std::string response = TabletWorkerSecondary::forward_to_primary(
                tablet_node, std::move(request));
            srw.write_line(response);
          }
        }
      }
    } else {
      // cannot parse the request
      srw.write_line("-ERR invalid request\r\n");
    }

    // close connection to frontend client
    srw.close_srw();
  }

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: secondary_frontend_handler thread" << std::endl;
    db_log(s.str());
  }
}

// secondary node contacting primary to update tablets files on disk
void TabletWorkerSecondary::update_tablets_files(TabletNode &tablet_node,
                                                 SocketReaderWriter &srw) {
  int tabid_to_ld = -1;
  for (auto tablet_itr = tablet_node.tablets.begin();
       tablet_itr != tablet_node.tablets.end(); ++tablet_itr) {
    // get MD5 hashes for tabid's checkpoint and log files
    std::vector<char> cp_md5;
    std::vector<char> lg_md5;
    bool op_result =
        tablet_node.get_tablet_hashes(tablet_itr->first, cp_md5, lg_md5);

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "Hashes of tablet <" << std::to_string(tablet_itr->first) << "> <"
        << std::boolalpha << op_result << ">" << std::endl;
      db_log(s.str());
    }

    // send MD5 hashes to primary for comparison
    srw.write_line("HASH " + std::to_string(tablet_itr->first) + "\r\n");
    srw.write_data(cp_md5);
    srw.write_data(lg_md5);

    std::string line;
    srw.read_line(line);

    // parse response from primary
    Request::SecPriRequest request;
    if (Parser::parse_secpri_request(line, request)) {
      if (request.type == Request::SecPriRequestType::none) {
        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Files for tabid <" << std::to_string(request.tabid)
            << "> already up to date" << std::endl;
          db_log(s.str());
        }

        continue;
      } else if (request.type == Request::SecPriRequestType::cp) {
        std::vector<char> cp_content;
        srw.read_data(request.cp_file_size, cp_content);
        bool op_result = tablet_node.update_tablet_file(
            request.tabid, cp_content, TabletFileType::cp);

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Updated files for tabid <" << std::to_string(request.tabid)
            << ">, CP <" << std::boolalpha << op_result << ">" << std::endl;
          db_log(s.str());
        }
      } else if (request.type == Request::SecPriRequestType::lg) {
        std::vector<char> lg_content;
        srw.read_data(request.lg_file_size, lg_content);
        bool op_result = tablet_node.update_tablet_file(
            request.tabid, lg_content, TabletFileType::lg);

        if (request.lg_file_size != 0) {
          tabid_to_ld = request.tabid;
          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Tabid <" << std::to_string(request.tabid)
              << "> LG file is not empty; need to load" << std::endl;
            db_log(s.str());
          }
        }

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Updated files for tabid <" << std::to_string(request.tabid)
            << ">, LG <" << std::boolalpha << op_result << ">" << std::endl;
          db_log(s.str());
        }
      } else if (request.type == Request::SecPriRequestType::both) {
        std::vector<char> cp_content;
        srw.read_data(request.cp_file_size, cp_content);
        bool op_result = tablet_node.update_tablet_file(
            request.tabid, cp_content, TabletFileType::cp);

        std::vector<char> lg_content;
        srw.read_data(request.lg_file_size, lg_content);
        bool op_result_2 = tablet_node.update_tablet_file(
            request.tabid, lg_content, TabletFileType::lg);

        if (request.lg_file_size != 0) {
          tabid_to_ld = request.tabid;
          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Tabid <" << std::to_string(request.tabid)
              << "> LG file is not empty; need to load" << std::endl;
            db_log(s.str());
          }
        }

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Updated files for tabid <" << std::to_string(request.tabid)
            << ">, CP <" << std::boolalpha << op_result << ">, LG <"
            << std::boolalpha << op_result_2 << ">" << std::endl;
          db_log(s.str());
        }
      }
    } else {
      {
        // DB
        std::stringstream s;
        s << "T" << tablet_node.index << " ";
        s << "Invalid request received from primary connection; update tablets "
             "files"
          << std::endl;
        db_log(s.str());
      }
    }
  }

  // tablet files in sync, load the first tablet
  if (tabid_to_ld == -1) {
    tabid_to_ld = tablet_node.tablets.begin()->first;
  }
  tablet_node.curr_tablet = nullptr;
  std::pair<bool, int> ld_result = tablet_node.load_tablet(tabid_to_ld);

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Load tabid <" << tabid_to_ld << "> <" << std::boolalpha
      << ld_result.first << "> initial size <" << tablet_node.tablet.size()
      << ">" << std::endl;
    db_log(s.str());
  }

  // replay log
  tablet_node.replay_log();
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Tabid <" << tablet_node.curr_tablet->id << "> log replayed, size <"
      << tablet_node.tablet.size() << ">" << std::endl;
    db_log(s.str());
  }

  // start receiving requests from primary
  srw.write_line("INIT\r\n");
}

// secondary node interfacing with primary node, for all updates
// blocked on recv()
void TabletWorkerSecondary::secondary_primary_handler(TabletNode &tablet_node,
                                                      bool is_restart) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New secondary_primary_handler thread" << std::endl;
    db_log(s.str());
  }

  // connect to primary
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int status =
      connect(sock,
              (const struct sockaddr *)&tablet_node.secondary_primary_conn_addr
                  .sock_addr,
              sizeof(tablet_node.secondary_primary_conn_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot connect: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  struct timeval tv;
  tv.tv_sec = 30;
  tv.tv_usec = 0;
  status = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot set socket timeout: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  SocketReaderWriter srw(sock);

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Connected to primary" << std::endl;
    db_log(s.str());
  }

  if (is_restart) {
    // restarting this secondary node
    // contact primary to update tablets files on disk

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "Restarting node, updating tablets files" << std::endl;
      db_log(s.str());
    }

    update_tablets_files(tablet_node, srw);
  } else {
    // initiating this secondary node
    // send primary an init message, start receiving requests from primary

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "Initiating new node, registering with primary" << std::endl;
      db_log(s.str());
    }

    srw.write_line("INIT\r\n");
  }

  while (!tablet_node.is_disabled) {
    // read and parse request
    std::string line;
    if (!srw.read_line(line)) {
      continue;
    }
    Request::Request request;

    if (Parser::parse_request(line, request)) {
      if (request.type == Request::RequestType::chec) {
        // checkpoint request

        int curr_tabid = tablet_node.curr_tablet->id;
        int req_tabid = request.tabid;

        if (req_tabid == curr_tabid) {
          // checkpoint if the requested tablet is currently in memory

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Current tabid <" << curr_tabid
              << ">, requested to checkpoint tabid <" << req_tabid << ">"
              << std::endl;
            db_log(s.str());
          }

          // checkpoint current tablet
          std::pair<bool, int> cp_result =
              tablet_node.checkpoint_tablet(curr_tabid);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Checkpoint <" << std::boolalpha << cp_result.first << ">"
              << std::endl;
            db_log(s.str());
          }
        }
      } else {
        // request operating on row and col

        // calculate tabid for the row
        int row_tabid = std::hash<std::string>{}(request.row) % 6;

        if (tablet_node.tablets.count(row_tabid) == 0) {
          // tabid is not served in this cluster
          srw.write_line("-ERR cannot find data; incorrect cluster\r\n");
        } else {
          // tabid is served in this cluster
          // need to make sure it is loaded in memory
          int curr_tabid = tablet_node.curr_tablet->id;

          if (row_tabid != curr_tabid) {
            // requested tablet is not in memory
            // need to checkpoint current tablet and load requested tablet

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "Current tabid <" << curr_tabid << ">, requested tabid <"
                << row_tabid << ">" << std::endl;
              db_log(s.str());
            }

            // checkpoint current tablet
            std::pair<bool, int> cp_result =
                tablet_node.checkpoint_tablet(curr_tabid);

            if (cp_result.first) {
              // checkpoint succeeded

              // send checkpoint sync request
              Request::Request chec_request;
              chec_request.str = "CHEC " + std::to_string(curr_tabid) + "\r\n";
              chec_request.type = Request::RequestType::chec;
              chec_request.tabid = curr_tabid;
              forward_to_primary(tablet_node, std::move(chec_request));

              // load requested tablet
              std::pair<bool, int> ld_result =
                  tablet_node.load_tablet(row_tabid);

              if (ld_result.first) {
                // requested tablet successfully loaded into memory
                {
                  // DB
                  std::stringstream s;
                  s << "T" << tablet_node.index << " ";
                  s << "Tablet <" << row_tabid << "> size <"
                    << tablet_node.tablet.size()
                    << "> after loading, prior to replay" << std::endl;
                  db_log(s.str());
                }

                tablet_node.replay_log();

                {
                  // DB
                  std::stringstream s;
                  s << "T" << tablet_node.index << " ";
                  s << "Tablet <" << row_tabid << "> size <"
                    << tablet_node.tablet.size() << "> after replay"
                    << std::endl;
                  db_log(s.str());
                }

                {
                  // DB
                  std::stringstream s;
                  s << "T" << tablet_node.index << " ";
                  s << "Checkpoint <" << curr_tabid << "> succeeded, loading <"
                    << row_tabid << "> succeeded, log replayed" << std::endl;
                  db_log(s.str());
                }
              } else {
                // loading did not occur
                // because tablet has already been loaded
                {
                  // DB
                  std::stringstream s;
                  s << "T" << tablet_node.index << " ";
                  s << "Checkpoint <" << curr_tabid
                    << "> succeeded, but loading <" << row_tabid
                    << "> did not occur because it has already been loaded"
                    << std::endl;
                  db_log(s.str());
                }
              }
            } else {
              // checkpoint did not succeed
              // because tablet has already been moved out of memory
              {
                // DB
                std::stringstream s;
                s << "T" << tablet_node.index << " ";
                s << "Checkpoint did not occur because <" << curr_tabid
                  << "> has already been moved out of memory" << std::endl;
                db_log(s.str());
              }
            }
          }

          // now, the requested tablet has been loaded in memory
          // perform the requests
          if (request.type == Request::RequestType::put) {
            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "Prior to PUT, tablet size <" << tablet_node.tablet.size()
                << ">" << std::endl;
              db_log(s.str());
            }

            srw.read_data(request.v1_size, request.v1);
            std::pair<bool, int> op_result = tablet_node.put(request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "PUT <" << std::boolalpha << op_result.first << ">"
                << std::endl;
              db_log(s.str());
            }

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "After PUT, tablet size <" << tablet_node.tablet.size()
                << ">" << std::endl;
              db_log(s.str());
            }
          } else if (request.type == Request::RequestType::cput) {
            srw.read_data(request.v1_size, request.v1);
            srw.read_data(request.v2_size, request.v2);
            std::pair<bool, int> op_result = tablet_node.cput(request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "CPUT <" << std::boolalpha << op_result.first << ">"
                << std::endl;
              db_log(s.str());
            }
          } else if (request.type == Request::RequestType::dele) {
            std::pair<bool, int> op_result = tablet_node.dele(request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "DELE <" << std::boolalpha << op_result.first << ">"
                << std::endl;
              db_log(s.str());
            }
          }
        }
      }
    } else {
      // invalid request will not be sent from primary
    }
  }

  // close connection to primary
  srw.close_srw();

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: secondary_primary_handler thread" << std::endl;
    db_log(s.str());
  }
}
