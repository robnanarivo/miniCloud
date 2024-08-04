#include "tablet_worker_primary.h"

#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <sstream>

#include "db_logger.h"
#include "parser.h"

// primary node interfacing with frontend clients and secondary nodes
// blocked on wait()
void TabletWorkerPrimary::primary_frontend_handler(
    TabletNode &tablet_node, TaskQueue &task_queue,
    RequestQueue &request_queue) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New primary_frontend_handler thread" << std::endl;
    db_log(s.str());
  }

  while (!tablet_node.is_disabled) {
    // get a socket connection from task queue
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
      } else if (request.type == Request::RequestType::chec) {
        // checkpoint request

        srw.write_line("+OK checkpoint request received\r\n");

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

          // forward checkpoint request to other nodes
          request_queue.enqueue(cp_result.second, request);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Checkpoint <" << std::boolalpha << cp_result.first
              << ">, seq# <" << cp_result.second << ">, enqueued" << std::endl;
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

            // forward checkpoint request to other nodes
            Request::Request chec_request;
            chec_request.str = "CHEC " + std::to_string(curr_tabid) + "\r\n";
            chec_request.type = Request::RequestType::chec;
            chec_request.tabid = curr_tabid;
            request_queue.enqueue(cp_result.second, chec_request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "CHEC, seq# <" << cp_result.second << ">, enqueued"
                << std::endl;
              db_log(s.str());
            }
          }

          // now, the requested tablet has been loaded in memory
          // perform the requests
          if (request.type == Request::RequestType::get) {
            std::vector<char> val;
            bool gt_result = tablet_node.get(request, val);
            if (gt_result) {
              srw.write_line("+OK " + std::to_string(val.size()) + "\r\n");
              srw.write_data(val);
            } else {
              srw.write_line("-ERR cannot find data; nonexistent\r\n");
            }
          } else if (request.type == Request::RequestType::put) {
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
            request_queue.enqueue(op_result.second, request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "PUT <" << std::boolalpha << op_result.first << ">, seq# <"
                << op_result.second << ">, enqueued" << std::endl;
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

            if (op_result.first) {
              srw.write_line("+OK PUT request succeeded\r\n");
            } else {
              srw.write_line("-ERR PUT request failed\r\n");
            }
          } else if (request.type == Request::RequestType::cput) {
            // handle request, add to request queue with sequence number
            srw.read_data(request.v1_size, request.v1);
            srw.read_data(request.v2_size, request.v2);
            std::pair<bool, int> op_result = tablet_node.cput(request);
            request_queue.enqueue(op_result.second, request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "CPUT <" << std::boolalpha << op_result.first << ">, seq# <"
                << op_result.second << ">, enqueued" << std::endl;
              db_log(s.str());
            }

            if (op_result.first) {
              srw.write_line("+OK CPUT request succeeded\r\n");
            } else {
              srw.write_line("-ERR CPUT request failed\r\n");
            }
          } else if (request.type == Request::RequestType::dele) {
            // handle request, add to request queue with sequence number
            std::pair<bool, int> op_result = tablet_node.dele(request);
            request_queue.enqueue(op_result.second, request);

            {
              // DB
              std::stringstream s;
              s << "T" << tablet_node.index << " ";
              s << "DELE <" << std::boolalpha << op_result.first << ">, seq# <"
                << op_result.second << ">, enqueued" << std::endl;
              db_log(s.str());
            }

            if (op_result.first) {
              srw.write_line("+OK DELE request succeeded\r\n");
            } else {
              srw.write_line("-ERR DELE request failed\r\n");
            }
          }
        }
      }
    } else {
      // cannot parse request
      srw.write_line("-ERR invalid request\r\n");
    }

    // close socket rw
    srw.close_srw();
  }

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: primary_frontend_handler thread" << std::endl;
    db_log(s.str());
  }
}

// primary node accepting connections from secondary nodes
// blocked on accept()
void TabletWorkerPrimary::primary_secondary_connection_handler(
    TabletNode &tablet_node, std::vector<SocketReaderWriter> &srws) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New primary_secondary_connection_handler thread" << std::endl;
    db_log(s.str());
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int opt = 1;
  int status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                          sizeof(opt));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot set socket opt: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = bind(
      sock,
      (struct sockaddr *)&tablet_node.secondary_primary_conn_addr.sock_addr,
      sizeof(tablet_node.secondary_primary_conn_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot bind to addr: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = listen(sock, 1);
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot listen: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  // struct for poll
  struct pollfd *pfds = (struct pollfd *)calloc(1, sizeof(*pfds));
  if (pfds == NULL) {
    // DB
    std::stringstream s;
    s << "Cannot calloc pollfd: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  pfds[0].fd = sock;
  pfds[0].events = POLLIN;

  while (!tablet_node.is_disabled) {
    status = poll(pfds, 1, 40000);
    if (status == 0) {
      // // DB
      // std::stringstream s;
      // s << "poll() timed out" << std::endl;
      // db_log(s.str());
      continue;
    } else if (status == -1) {
      // DB
      std::stringstream s;
      s << "Cannot poll: " << strerror(errno) << std::endl;
      db_log(s.str());
      continue;
    }

    if (!(pfds[0].revents & POLLIN)) {
      // DB
      std::stringstream s;
      s << "poll() returned error event: " << strerror(errno) << std::endl;
      db_log(s.str());
      continue;
    }

    int conn = accept(sock, NULL, NULL);
    if (conn <= 0) {
      // DB
      std::stringstream s;
      s << "Cannot accept connection: " << strerror(errno) << std::endl;
      db_log(s.str());
      continue;
    }

    SocketReaderWriter srw(conn);

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "Recieved a new connection from a secondary" << std::endl;
      db_log(s.str());
    }

    // read and parse connection request from secondary
    while (true) {
      std::string line;
      srw.read_line(line);
      Request::SecPriRequest request;

      if (Parser::parse_secpri_request(line, request)) {
        if (request.type == Request::SecPriRequestType::init) {
          // secondary initiated, add to vector
          break;
        } else if (request.type == Request::SecPriRequestType::hash) {
          // secondary is restarting, need to update files

          // read hashes
          std::vector<char> cp_md5_sec;
          srw.read_data(16, cp_md5_sec);
          std::vector<char> lg_md5_sec;
          srw.read_data(16, lg_md5_sec);
          // get hashes of local copies
          std::vector<char> cp_md5_local;
          std::vector<char> lg_md5_local;
          bool op_result = tablet_node.get_tablet_hashes(
              request.tabid, cp_md5_local, lg_md5_local);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Hashes of tablet <" << std::to_string(request.tabid)
              << "> in local copy <" << std::boolalpha << op_result << ">"
              << std::endl;
            db_log(s.str());
          }

          bool cp_hashes_equal = cp_md5_sec == cp_md5_local;
          bool lg_hashes_equal = lg_md5_sec == lg_md5_local;

          if (cp_hashes_equal && lg_hashes_equal) {
            // secondary's both files are up to date
            srw.write_line("NONE " + std::to_string(request.tabid) + "\r\n");
          } else if (!cp_hashes_equal && !lg_hashes_equal) {
            // secondary's both files need to be updated

            // retrieve files' contents
            std::vector<char> cp_content;
            bool op_result = tablet_node.get_tablet_file(
                request.tabid, cp_content, TabletFileType::cp);

            std::vector<char> lg_content;
            bool op_result_2 = tablet_node.get_tablet_file(
                request.tabid, lg_content, TabletFileType::lg);

            srw.write_line("BOTH " + std::to_string(request.tabid) + " " +
                           std::to_string(cp_content.size()) + " " +
                           std::to_string(lg_content.size()) + "\r\n");
            srw.write_data(cp_content);
            srw.write_data(lg_content);
          } else if (!cp_hashes_equal) {
            // secondary's checkpoint file needs to be updated

            // retrieve checkpoint file's contents
            std::vector<char> cp_content;
            bool op_result = tablet_node.get_tablet_file(
                request.tabid, cp_content, TabletFileType::cp);

            srw.write_line("CP " + std::to_string(request.tabid) + " " +
                           std::to_string(cp_content.size()) + "\r\n");
            srw.write_data(cp_content);
          } else if (!lg_hashes_equal) {
            // secondary's log file needs to be updated

            // retrieve log file's contents
            std::vector<char> lg_content;
            bool op_result = tablet_node.get_tablet_file(
                request.tabid, lg_content, TabletFileType::lg);

            srw.write_line("LG " + std::to_string(request.tabid) + " " +
                           std::to_string(lg_content.size()) + "\r\n");
            srw.write_data(lg_content);
          }
        }
      } else {
        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Invalid request received from secondary connection; "
               "connection"
            << std::endl;
          db_log(s.str());
        }
        break;
      }
    }

    // move the new socket connection to vector
    srws.push_back(std::move(srw));

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "Added a new secondary connection" << std::endl;
      db_log(s.str());
    }
  }

  close(sock);
  free(pfds);

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: primary_secondary_connection_handler thread" << std::endl;
    db_log(s.str());
  }
}

// primary node interfacing with secondary nodes
// blocked on wait()
void TabletWorkerPrimary::primary_secondary_handler(
    TabletNode &tablet_node, RequestQueue &request_queue,
    std::vector<SocketReaderWriter> &srws) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New primary_secondary_handler thread" << std::endl;
    db_log(s.str());
  }

  while (!tablet_node.is_disabled) {
    // get a request from synchronized queue
    int seq;
    Request::Request request;
    if (!request_queue.deque(request, seq)) {
      // {
      //   // DB
      //   std::stringstream s;
      //   s << "T" << tablet_node.index << " ";
      //   s << "Request queue timed out" << std::endl;
      //   db_log(s.str());
      // }
      continue;
    }

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "New update request, seq# <" << seq << "> dequed: " << request.str;
      db_log(s.str());
    }

    // send request to all secondary nodes
    int counter = 0;
    if (request.type == Request::RequestType::put) {
      for (auto srw_itr = srws.begin(); srw_itr != srws.end();) {
        bool status = srw_itr->write_line(request.str);
        if (status) {
          srw_itr->write_data(request.v1);
          ++srw_itr;
          ++counter;
        } else {
          srw_itr = srws.erase(srw_itr);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Removed a secondary connection" << std::endl;
            db_log(s.str());
          }
        }
      }
    } else if (request.type == Request::RequestType::cput) {
      for (auto srw_itr = srws.begin(); srw_itr != srws.end();) {
        bool status = srw_itr->write_line(request.str);
        if (status) {
          srw_itr->write_data(request.v1);
          srw_itr->write_data(request.v2);
          ++srw_itr;
          ++counter;
        } else {
          srw_itr = srws.erase(srw_itr);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Removed a secondary connection" << std::endl;
            db_log(s.str());
          }
        }
      }
    } else if (request.type == Request::RequestType::dele) {
      for (auto srw_itr = srws.begin(); srw_itr != srws.end();) {
        bool status = srw_itr->write_line(request.str);
        if (status) {
          ++srw_itr;
          ++counter;
        } else {
          srw_itr = srws.erase(srw_itr);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Removed a secondary connection" << std::endl;
            db_log(s.str());
          }
        }
      }
    } else if (request.type == Request::RequestType::chec) {
      for (auto srw_itr = srws.begin(); srw_itr != srws.end();) {
        bool status = srw_itr->write_line(request.str);
        if (status) {
          ++srw_itr;
          ++counter;
        } else {
          srw_itr = srws.erase(srw_itr);

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Removed a secondary connection" << std::endl;
            db_log(s.str());
          }
        }
      }
    }

    {
      // DB
      std::stringstream s;
      s << "T" << tablet_node.index << " ";
      s << "Update request, seq# <" << seq << "> forwarded to " << counter
        << " secondaries" << std::endl;
      db_log(s.str());
    }
  }

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: primary_secondary_handler thread" << std::endl;
    db_log(s.str());
  }
}
