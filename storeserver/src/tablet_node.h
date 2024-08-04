#pragma once

#include <atomic>
#include <fstream>
#include <map>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "address.h"
#include "request.h"

// struct for a tablet
struct Tablet {
  int id;
  std::string cp_file;
  std::string lg_file;
};

enum class TabletFileType { cp, lg };

class TabletNode {
 public:
  // atomic bool indicating whether this node is disabled
  std::atomic_bool is_disabled = false;

  // node index in cluster
  int index;

  // number of updates since server started
  int seq_num_counter = 0;
  // number of updates since last checkpoint
  int update_counter = 0;
  // number of updates between checkpoints
  int cp_freq = 100;

  // file descriptor of current log file, and mtx
  int curr_lg_fd;
  std::mutex log_mtx;

  // current tablet in memory, and mtx on tablet and rows
  Tablet *curr_tablet = nullptr;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::vector<char>>>
      tablet;
  std::shared_mutex tablet_mtx;
  std::unordered_map<std::string, std::shared_mutex> rows_mtx;

  // all tablets on this server
  std::unordered_map<int, Tablet> tablets;

  // whether this is the primary node
  bool is_primary;

  // addr of master node
  Address master_addr;

  // addr to accept frontend connections
  Address frontend_bind_addr;

  // for secondary node, addr to forward update requests
  Address primary_addr;

  // for secondary node, addr to connect to primary node
  Address secondary_primary_conn_addr;

  // add a new tablet
  // return a bool indicating whether the insertion happened
  bool add_tablet(int tabid);

  // write checkpoint for a tablet
  // return a pair: bool indicating whether checkpoint is performed, int
  // indicating sequence number if tabid exists, 0 if tabid is invalid
  std::pair<bool, int> checkpoint_tablet(int tabid, bool locking = true);

  // load a tablet
  // return a pair: bool indicating whether loading is performed, int
  // indicating tabid after this operation
  std::pair<bool, int> load_tablet(int tabid);

  // replay update operations in current tablet's log file
  void replay_log();

  // get MD5 hashes of checkpoint and log files of the tabid
  // return bool indicating whether the operation succeeds
  // return hashes via parameter
  bool get_tablet_hashes(int tabid, std::vector<char> &cp_md5,
                         std::vector<char> &lg_md5);

  // get a tablet file from disk
  // return bool indicating whether the operation succeeds
  // return file content via parameter
  bool get_tablet_file(int tabid, std::vector<char> &file_content,
                       TabletFileType type);

  // update a tablet file on disk
  // return bool indicating whether the operation succeeds
  bool update_tablet_file(int tabid, std::vector<char> &file_content,
                          TabletFileType type);

  // get all row_col pairs in memory and on disk
  // return bool indicating whether the operation succeeds
  // return all pairs via parameter
  bool get_raw(std::multimap<std::string, std::string> &rows_keys);

  // store value in row and column
  // return a pair: bool indicating whether operation is performed, int
  // indicating sequence number of this operation
  std::pair<bool, int> put(const Request::Request &request,
                           bool locking = true);

  // store value in row and column if current value is v1
  // return a pair: bool indicating whether operation is performed, int
  // indicating sequence number of this operation
  std::pair<bool, int> cput(const Request::Request &request,
                            bool locking = true);

  // delete value in row and column
  // return a pair: bool indicating whether operation is performed, int
  // indicating sequence number of this operation
  std::pair<bool, int> dele(const Request::Request &request,
                            bool locking = true);

  // return value stored in row and column through parameter val
  bool get(const Request::Request &request, std::vector<char> &val);

  // clear memory and states
  void clear();
};
