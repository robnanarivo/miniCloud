#pragma once

#include "request_queue.h"
#include "socket_reader_writer.h"
#include "tablet_node.h"
#include "task_queue.h"

namespace TabletWorkerPrimary {
// primary node interfacing with frontend clients and secondary nodes
void primary_frontend_handler(TabletNode &tablet_node, TaskQueue &task_queue,
                              RequestQueue &request_queue);

// primary node accepting connections from secondary nodes
void primary_secondary_connection_handler(
    TabletNode &tablet_node, std::vector<SocketReaderWriter> &srws);

// primary node interfacing with secondary nodes
void primary_secondary_handler(TabletNode &tablet_node,
                               RequestQueue &request_queue,
                               std::vector<SocketReaderWriter> &srws);

};  // namespace TabletWorkerPrimary