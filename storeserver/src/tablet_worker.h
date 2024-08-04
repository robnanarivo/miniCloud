#pragma once

#include <thread>
#include <vector>

#include "request_queue.h"
#include "socket_reader_writer.h"
#include "tablet_node.h"
#include "task_queue.h"

namespace TabletWorker {
// nodes accepting connections from frontend clients
void frontend_connections_handler(TabletNode &tablet_node,
                                  TaskQueue &task_queue);

// helper function for creating thread pool, called by master_handler
void populate_thread_pool(int thread_pool_size,
                          std::vector<std::thread> &thread_pool,
                          TabletNode &tablet_node, TaskQueue &task_queue,
                          RequestQueue &request_queue,
                          std::vector<SocketReaderWriter> &srws,
                          bool is_restart);

// interfacing with master
void master_handler(TabletNode &tablet_node, int thread_pool_size,
                    bool is_restart);

// sending heartbeat msgs to master
void heartbeat_handler(TabletNode &tablet_node);

};  // namespace TabletWorker