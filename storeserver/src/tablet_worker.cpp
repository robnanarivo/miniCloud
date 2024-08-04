#include "tablet_worker.h"

#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <sstream>
#include <thread>

#include "db_logger.h"
#include "parser.h"
#include "tablet_worker_primary.h"
#include "tablet_worker_secondary.h"

// nodes accepting connections from frontend clients
// blocked on accept()
void TabletWorker::frontend_connections_handler(TabletNode &tablet_node,
                                                TaskQueue &task_queue) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New frontend_connections_handler thread" << std::endl;
    db_log(s.str());
  }

  int frontend_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (frontend_sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int opt = 1;
  int status = setsockopt(frontend_sock, SOL_SOCKET,
                          SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot set socket opt: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = bind(frontend_sock,
                (struct sockaddr *)&tablet_node.frontend_bind_addr.sock_addr,
                sizeof(tablet_node.frontend_bind_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot bind to addr: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  status = listen(frontend_sock, 100);
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

  pfds[0].fd = frontend_sock;
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

    if (pfds[0].revents & POLLIN) {
      int new_conn = accept(frontend_sock, NULL, NULL);
      if (new_conn <= 0) {
        // DB
        std::stringstream s;
        s << "Cannot accept connection: " << strerror(errno) << std::endl;
        db_log(s.str());
      } else {
        task_queue.enqueue(new_conn);
      }
    } else {
      // DB
      std::stringstream s;
      s << "poll() returned error event: " << strerror(errno) << std::endl;
      db_log(s.str());
    }
  }

  // clean up
  close(frontend_sock);
  free(pfds);

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: frontend_connections_handler thread" << std::endl;
    db_log(s.str());
  }
}

// helper function for creating thread pool, called by master_handler
void TabletWorker::populate_thread_pool(
    int thread_pool_size, std::vector<std::thread> &thread_pool,
    TabletNode &tablet_node, TaskQueue &task_queue, RequestQueue &request_queue,
    std::vector<SocketReaderWriter> &srws, bool is_restart) {
  // start thread for sending heartbeat msg
  thread_pool.push_back(
      std::thread(TabletWorker::heartbeat_handler, std::ref(tablet_node)));

  // start thread for accepting frontend connections
  thread_pool.push_back(std::thread(TabletWorker::frontend_connections_handler,
                                    std::ref(tablet_node),
                                    std::ref(task_queue)));

  if (tablet_node.is_primary) {
    // start frontend-interfacing threads
    for (int i = 0; i != thread_pool_size; ++i) {
      thread_pool.push_back(std::thread(
          TabletWorkerPrimary::primary_frontend_handler, std::ref(tablet_node),
          std::ref(task_queue), std::ref(request_queue)));
    }
    // start secondary-connection thread
    thread_pool.push_back(
        std::thread(TabletWorkerPrimary::primary_secondary_connection_handler,
                    std::ref(tablet_node), std::ref(srws)));
    // start secondary-interfacing thread
    thread_pool.push_back(std::thread(
        TabletWorkerPrimary::primary_secondary_handler, std::ref(tablet_node),
        std::ref(request_queue), std::ref(srws)));

  } else {
    // start frontend-interfacing threads
    for (int i = 0; i != thread_pool_size; ++i) {
      thread_pool.push_back(
          std::thread(TabletWorkerSecondary::secondary_frontend_handler,
                      std::ref(tablet_node), std::ref(task_queue)));
    }
    // start primary-interfacing thread
    thread_pool.push_back(
        std::thread(TabletWorkerSecondary::secondary_primary_handler,
                    std::ref(tablet_node), is_restart));
  }
}

// interfacing with master server
void TabletWorker::master_handler(TabletNode &tablet_node, int thread_pool_size,
                                  bool is_restart) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New master_handler thread" << std::endl;
    db_log(s.str());
  }

  // create data structures
  std::vector<std::thread> thread_pool;
  TaskQueue task_queue;
  RequestQueue request_queue;
  std::vector<SocketReaderWriter> srws;

  // connect to master server
  int master_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (master_sock == -1) {
    // DB
    std::stringstream s;
    s << "Cannot create socket: " << strerror(errno) << std::endl;
    db_log(s.str());
  }
  int status = connect(
      master_sock, (const struct sockaddr *)&tablet_node.master_addr.sock_addr,
      sizeof(tablet_node.master_addr.sock_addr));
  if (status == -1) {
    // DB
    std::stringstream s;
    s << "Cannot connect: " << strerror(errno) << std::endl;
    db_log(s.str());
  }

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Connected to master" << std::endl;
    db_log(s.str());
  }

  SocketReaderWriter srw(master_sock);

  // send a join msg to master
  srw.write_line("JOIN " + tablet_node.frontend_bind_addr.sock_addr_str +
                 "\r\n");

  while (true) {
    // read and parse request from master
    std::string line;
    srw.read_line(line);
    Request::MaTabRequest request;

    if (Parser::parse_matab_request(line, request)) {
      if (request.type == Request::MaTabRequestType::add) {
        // add a new tablet
        bool op_result = tablet_node.add_tablet(request.tabid);

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Add tabid <" << request.tabid << "> <" << std::boolalpha
            << op_result << ">" << std::endl;
          db_log(s.str());
        }

        // load the first tablet
        if (tablet_node.curr_tablet == nullptr) {
          std::pair<bool, int> ld_result =
              tablet_node.load_tablet(request.tabid);
          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Load tabid <" << request.tabid << "> <" << std::boolalpha
              << ld_result.first << "> initial size <"
              << tablet_node.tablet.size() << ">" << std::endl;
            db_log(s.str());
          }

          // replay log
          tablet_node.replay_log();
          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Tabid <" << tablet_node.curr_tablet->id
              << "> log replayed, size <" << tablet_node.tablet.size() << ">"
              << std::endl;
            db_log(s.str());
          }
        }
      } else if (request.type == Request::MaTabRequestType::init) {
        // received all meta info from master, create threads
        TabletWorker::populate_thread_pool(thread_pool_size, thread_pool,
                                           tablet_node, task_queue,
                                           request_queue, srws, is_restart);

        {
          // DB: Master node meta info
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "All threads created: " << thread_pool.size() << std::endl;
          db_log(s.str());
        }
      } else if (request.type == Request::MaTabRequestType::kill) {
        // set atomic flag to true
        tablet_node.is_disabled = true;

        // send signal to thread, join, remove from thread pool
        for (auto t_itr = thread_pool.begin(); t_itr != thread_pool.end();) {
          t_itr->join();
          t_itr = thread_pool.erase(t_itr);
        }

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Node disabled by admin console" << std::endl;
          db_log(s.str());
        }

        // clear data structure and memory
        task_queue.clear();
        request_queue.clear();
        srws.clear();
        tablet_node.clear();

        {
          // DB
          std::stringstream s;
          s << "T" << tablet_node.index << " ";
          s << "Cleared all data structures and states" << std::endl;
          db_log(s.str());
        }
      } else if (request.type == Request::MaTabRequestType::rest) {
        // resend a join msg to master
        if (tablet_node.is_disabled) {
          is_restart = true;
          tablet_node.is_disabled = false;

          // close old sock connection
          close(master_sock);

          // connect to master server
          master_sock = socket(AF_INET, SOCK_STREAM, 0);
          if (master_sock == -1) {
            // DB
            std::stringstream s;
            s << "Cannot create socket: " << strerror(errno) << std::endl;
            db_log(s.str());
          }
          status = connect(
              master_sock,
              (const struct sockaddr *)&tablet_node.master_addr.sock_addr,
              sizeof(tablet_node.master_addr.sock_addr));
          if (status == -1) {
            // DB
            std::stringstream s;
            s << "Cannot connect: " << strerror(errno) << std::endl;
            db_log(s.str());
          }

          {
            // DB
            std::stringstream s;
            s << "T" << tablet_node.index << " ";
            s << "Connected to master" << std::endl;
            db_log(s.str());
          }

          srw = SocketReaderWriter(master_sock);

          // send a join msg to master
          srw.write_line("JOIN " +
                         tablet_node.frontend_bind_addr.sock_addr_str + "\r\n");
        }
      }
    } else {
      // error: invalid control request
    }
  }
}

// sending heartbeat msgs to master
// blocked on sleep()
void TabletWorker::heartbeat_handler(TabletNode &tablet_node) {
  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "New heartbeat_handler thread" << std::endl;
    db_log(s.str());
  }

  while (!tablet_node.is_disabled) {
    // make a connection to master
    int master_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (master_sock == -1) {
      // DB
      std::stringstream s;
      s << "Cannot create socket: " << strerror(errno) << std::endl;
      db_log(s.str());
    }
    int status =
        connect(master_sock,
                (const struct sockaddr *)&tablet_node.master_addr.sock_addr,
                sizeof(tablet_node.master_addr.sock_addr));
    if (status == -1) {
      // DB
      std::stringstream s;
      s << "Cannot connect: " << strerror(errno) << std::endl;
      db_log(s.str());
    }

    // report alive status
    SocketReaderWriter srw(master_sock);
    srw.write_line("STAT " + tablet_node.frontend_bind_addr.sock_addr_str +
                   "\r\n");
    srw.close_srw();

    // wait for 100 seconds until next heartbeat msg
    std::this_thread::sleep_for(std::chrono::seconds(40));
  }

  {
    // DB
    std::stringstream s;
    s << "T" << tablet_node.index << " ";
    s << "Exited: heartbeat_handler thread" << std::endl;
    db_log(s.str());
  }
}
