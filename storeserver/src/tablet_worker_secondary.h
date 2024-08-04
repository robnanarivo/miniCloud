#pragma once

#include "request.h"
#include "socket_reader_writer.h"
#include "tablet_node.h"
#include "task_queue.h"

namespace TabletWorkerSecondary {
// forward request to parimary node
std::string forward_to_primary(TabletNode &tablet_node,
                               Request::Request request);

// secondary node interfacing with frontend clients
void secondary_frontend_handler(TabletNode &tablet_node, TaskQueue &task_queue);

// secondary node contacting primary to update tablets files on disk
void update_tablets_files(TabletNode &tablet_node, SocketReaderWriter &srw);

// secondary node interfacing with primary node
void secondary_primary_handler(TabletNode &tablet_node, bool is_restart);

};  // namespace TabletWorkerSecondary