#include "tablet_node.h"

#include <fcntl.h>
#include <openssl/md5.h>
#include <unistd.h>

#include <mutex>

#include "parser.h"
#include "tablet_node.h"

// add a new tablet
// return a bool indicating whether the insertion happened
bool TabletNode::add_tablet(int tabid) {
  Tablet tablet;
  tablet.id = tabid;
  tablet.cp_file = frontend_bind_addr.sock_addr_str + "_tablet" +
                   std::to_string(tabid) + "_cp.txt";
  tablet.lg_file = frontend_bind_addr.sock_addr_str + "_tablet" +
                   std::to_string(tabid) + "_lg.txt";
  return tablets.emplace(std::make_pair(tabid, std::move(tablet))).second;
}

// write checkpoint for a tablet
// return a pair: bool indicating whether checkpoint is performed, int
// indicating sequence number if tabid exists, -1 if tabid is invalid
std::pair<bool, int> TabletNode::checkpoint_tablet(int tabid, bool locking) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);

  int seq_num = 0;

  if (locking) {
    std::lock(log_lck, tablet_lck);
    if (tablets.find(tabid) == tablets.end()) {
      // tabid is not valid, return (false, 0)
      return std::make_pair(false, -1);
    } else if (curr_tablet->id != tabid) {
      // tabid is valid but not in memory, checkpoint not needed on this node
      // but may be needed on other nodes
      seq_num = ++seq_num_counter;
      return std::make_pair(false, seq_num);
    } else {
      // tabid is currently in memory, perform checkpoint
      seq_num = ++seq_num_counter;
    }
  } else {
    // called by self, must do checkpoint, must return (true,0)
  }

  // create temporary file
  std::string temp_name(curr_tablet->cp_file + "_new");
  int temp_fd = open(temp_name.c_str(), O_RDWR | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  // write tablet meta info "num_rows\n"
  std::string tablet_info(std::to_string(tablet.size()) + "\n");
  write(temp_fd, tablet_info.c_str(), sizeof(char) * tablet_info.size());

  for (auto row_itr = tablet.begin(); row_itr != tablet.end(); ++row_itr) {
    // write row meta info "row_key num_cols\n"
    std::string row_info(row_itr->first + " " +
                         std::to_string(row_itr->second.size()) + "\n");
    write(temp_fd, row_info.c_str(), sizeof(char) * row_info.size());

    // write col meta info "col_key num_bytes\ndata"
    for (auto col_itr = row_itr->second.begin();
         col_itr != row_itr->second.end(); ++col_itr) {
      std::string col_info(col_itr->first + " " +
                           std::to_string(col_itr->second.size()) + "\n");
      write(temp_fd, col_info.c_str(), sizeof(char) * col_info.size());
      write(temp_fd, &(col_itr->second[0]), col_itr->second.size());
    }
  }
  fsync(temp_fd);

  // rename and close temp file
  rename(temp_name.c_str(), curr_tablet->cp_file.c_str());
  close(temp_fd);

  // empty log file, reset update count
  ftruncate(curr_lg_fd, 0);
  fsync(curr_lg_fd);
  update_counter = 0;

  return std::make_pair(true, seq_num);
}

// load a tablet
// return a pair: bool indicating whether loading is performed, int
// indicating tabid after this operation
std::pair<bool, int> TabletNode::load_tablet(int tabid) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);
  std::lock(log_lck, tablet_lck);

  // set current tablet
  auto itr = tablets.find(tabid);
  if (itr == tablets.end()) {
    return std::make_pair(false, -1);
  } else if (curr_tablet != nullptr && curr_tablet->id == tabid) {
    return std::make_pair(false, curr_tablet->id);
  }
  curr_tablet = &itr->second;

  // clear in-memory tablet, rows mutexes, close lg file
  tablet.clear();
  rows_mtx.clear();
  update_counter = 0;
  close(curr_lg_fd);

  // open tablet's checkpoint file
  std::ifstream cp_stream(curr_tablet->cp_file);

  // get num of rows
  int num_rows = 0;
  cp_stream >> num_rows;

  for (int i = 0; i < num_rows; i++) {
    // get row key and num of cols
    std::string row_key;
    int num_cols;
    cp_stream >> row_key >> num_cols;

    std::unordered_map<std::string, std::vector<char>> row;
    for (int j = 0; j < num_cols; j++) {
      // get col_key and num of bytes
      std::string col_key;
      int num_bytes;
      cp_stream >> col_key >> num_bytes >> std::ws;

      std::vector<char> data(num_bytes);
      cp_stream.read(&data[0], num_bytes);

      // Avoid copying
      row.emplace(std::make_pair(col_key, std::move(data)));
    }

    tablet.emplace(std::make_pair(row_key, std::move(row)));
    rows_mtx.try_emplace(row_key);
  }
  cp_stream.close();

  // load log file
  curr_lg_fd = open(curr_tablet->lg_file.c_str(), O_RDWR | O_CREAT | O_APPEND,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  return std::make_pair(true, curr_tablet->id);
}

// replay update operations in current tablet's log file
void TabletNode::replay_log() {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);
  std::lock(log_lck, tablet_lck);

  // open current tablet's log file
  std::ifstream log_stream(curr_tablet->lg_file);

  for (std::string line; std::getline(log_stream, line, '\n');) {
    // parse request
    Request::Request request;
    Parser::parse_request(line.append("\n"), request);

    // perform corresponding operations, indicating that no lock is needed
    if (request.type == Request::RequestType::put) {
      request.v1.resize(request.v1_size);
      log_stream.read(&request.v1[0], request.v1_size);
      put(request, false);
    } else if (request.type == Request::RequestType::cput) {
      request.v1.resize(request.v1_size);
      request.v2.resize(request.v2_size);
      log_stream.read(&request.v1[0], request.v1_size);
      log_stream.read(&request.v2[0], request.v2_size);
      cput(request, false);
    } else if (request.type == Request::RequestType::dele) {
      dele(request, false);
    }
  }

  log_stream.close();
}

// get MD5 hashes of checkpoint and log files of the tabid
// return bool indicating whether the operation succeeds
// return hashes via parameter
bool TabletNode::get_tablet_hashes(int tabid, std::vector<char> &cp_md5,
                                   std::vector<char> &lg_md5) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);
  std::lock(log_lck, tablet_lck);

  // find tablet
  auto tablet_itr = tablets.find(tabid);

  if (tablet_itr == tablets.end()) {
    return false;
  }

  std::ifstream cp_file(tablet_itr->second.cp_file, std::ios::binary);
  std::ifstream lg_file(tablet_itr->second.lg_file, std::ios::binary);

  std::istreambuf_iterator<char> cp_start(cp_file), cp_end;
  std::vector<char> cp_content(cp_start, cp_end);
  std::istreambuf_iterator<char> lg_start(lg_file), lg_end;
  std::vector<char> lg_content(lg_start, lg_end);

  std::vector<unsigned char> cp_md5_unsigned(16);
  std::vector<unsigned char> lg_md5_unsigned(16);

  MD5_CTX cp_hash;
  MD5_Init(&cp_hash);
  MD5_Update(&cp_hash, &cp_content[0], cp_content.size());
  MD5_Final(&cp_md5_unsigned[0], &cp_hash);

  MD5_CTX lg_hash;
  MD5_Init(&lg_hash);
  MD5_Update(&lg_hash, &lg_content[0], lg_content.size());
  MD5_Final(&lg_md5_unsigned[0], &lg_hash);

  cp_md5 = std::vector<char>(cp_md5_unsigned.begin(), cp_md5_unsigned.end());
  lg_md5 = std::vector<char>(lg_md5_unsigned.begin(), lg_md5_unsigned.end());

  cp_file.close();
  lg_file.close();

  return true;
}

// get a tablet file from disk
// return bool indicating whether the operation succeeds
// return file content via parameter
bool TabletNode::get_tablet_file(int tabid, std::vector<char> &file_content,
                                 TabletFileType type) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);
  std::lock(log_lck, tablet_lck);

  // find tablet
  auto tablet_itr = tablets.find(tabid);

  if (tablet_itr == tablets.end()) {
    return false;
  }

  std::ifstream file;

  if (type == TabletFileType::cp) {
    file.open(tablet_itr->second.cp_file);
  } else if (type == TabletFileType::lg) {
    file.open(tablet_itr->second.lg_file);
  }

  std::istreambuf_iterator<char> start(file), end;
  std::vector<char> content(start, end);
  file_content = std::move(content);

  file.close();

  return true;
}

// update a tablet file on disk
// return bool indicating whether the operation succeeds
bool TabletNode::update_tablet_file(int tabid, std::vector<char> &file_content,
                                    TabletFileType type) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);
  std::lock(log_lck, tablet_lck);

  // find tablet
  auto tablet_itr = tablets.find(tabid);

  if (tablet_itr == tablets.end()) {
    return false;
  }

  std::ofstream new_file;

  if (type == TabletFileType::cp) {
    new_file.open(tablet_itr->second.cp_file, std::ios_base::trunc);
  } else if (type == TabletFileType::lg) {
    new_file.open(tablet_itr->second.lg_file, std::ios_base::trunc);
  }

  new_file.write(&file_content[0], file_content.size());
  new_file.close();

  return true;
}

// store value in row and column, return sequence number of this operation
std::pair<bool, int> TabletNode::put(const Request::Request &request,
                                     bool locking) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);

  int seq_num = 0;

  if (locking) {
    std::lock(log_lck, tablet_lck);

    // logging
    seq_num = ++seq_num_counter;
    write(curr_lg_fd, request.str.c_str(), sizeof(char) * request.str.size());
    write(curr_lg_fd, &request.v1[0], request.v1_size);
    fsync(curr_lg_fd);
    if (++update_counter % cp_freq != 0) {
      // do not need checkpoint, release log lock
      log_lck.unlock();
    }
  }

  auto row_mtx_itr = rows_mtx.find(request.row);

  // add row and mutex
  if (row_mtx_itr == rows_mtx.end()) {
    // insert if row key does not exist
    row_mtx_itr = rows_mtx.try_emplace(request.row).first;
    tablet.try_emplace(request.row);
  }

  // acquire exclusive access to row
  std::unique_lock row_lck(row_mtx_itr->second);
  auto row_itr = tablet.find(request.row);

  // insert a new col and val, or assign val as a new value
  row_itr->second.insert_or_assign(request.col, request.v1);

  // checkpoint if hasn't released log_mtx
  if (locking && log_lck.owns_lock()) {
    // have exclusive access to log file and tablet
    checkpoint_tablet(curr_tablet->id, false);
  }

  return std::make_pair(true, seq_num);
}

// get all row_col pairs in memory and on disk
// return bool indicating whether the operation succeeds
// return all pairs via parameter
bool TabletNode::get_raw(std::multimap<std::string, std::string> &rows_keys) {
  // exclusive locks on log and tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::unique_lock tablet_lck(tablet_mtx, std::defer_lock);
  std::lock(log_lck, tablet_lck);

  // add all row_col pairs in memory
  for (auto row_itr = tablet.begin(); row_itr != tablet.end(); ++row_itr) {
    for (auto col_itr = row_itr->second.begin();
         col_itr != row_itr->second.end(); ++col_itr) {
      rows_keys.insert({row_itr->first, col_itr->first});
    }
  }

  // parse all row_col pairs in tablets on disk
  for (auto tablet_itr = tablets.begin(); tablet_itr != tablets.end();
       ++tablet_itr) {
    if (tablet_itr->second.id == curr_tablet->id) {
      continue;
    }
    // open tablet's checkpoint file
    std::ifstream cp_stream(tablet_itr->second.cp_file);

    // get num of rows
    int num_rows = 0;
    cp_stream >> num_rows;

    for (int i = 0; i < num_rows; i++) {
      // get row key and num of cols
      std::string row_key;
      int num_cols;
      cp_stream >> row_key >> num_cols;

      for (int j = 0; j < num_cols; j++) {
        // get col_key and num of bytes
        std::string col_key;
        int num_bytes;
        cp_stream >> col_key >> num_bytes >> std::ws;
        cp_stream.ignore(num_bytes);

        rows_keys.insert({row_key, col_key});
      }
    }
    cp_stream.close();
  }

  return true;
}

// store value in row and column if current value is v1, return sequence
// number of this operation
std::pair<bool, int> TabletNode::cput(const Request::Request &request,
                                      bool locking) {
  // exclusive lock on log, shared lock tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::shared_lock tablet_lck(tablet_mtx, std::defer_lock);

  bool success = false;
  int seq_num = 0;

  if (locking) {
    std::lock(log_lck, tablet_lck);

    // logging
    seq_num = ++seq_num_counter;
    write(curr_lg_fd, request.str.c_str(), sizeof(char) * request.str.size());
    write(curr_lg_fd, &request.v1[0], request.v1_size);
    write(curr_lg_fd, &request.v2[0], request.v2_size);
    fsync(curr_lg_fd);
    if (++update_counter % cp_freq != 0) {
      // do not need checkpoint, release log lock
      log_lck.unlock();
    }
  }

  auto row_mtx_itr = rows_mtx.find(request.row);

  if (row_mtx_itr != rows_mtx.end()) {
    // acquire exclusive access to row
    std::unique_lock row_lck(row_mtx_itr->second);
    auto row_itr = tablet.find(request.row);

    // find col and compare current value with v1
    auto val_itr = row_itr->second.find(request.col);
    if (val_itr != row_itr->second.end()) {
      // if current value == v1, update value to v2
      if (val_itr->second == request.v1) {
        val_itr->second = request.v2;
        success = true;
      }
    }
  }

  // checkpoint if hasn't released log_mtx
  if (locking && log_lck.owns_lock()) {
    checkpoint_tablet(curr_tablet->id, false);
  }

  return std::make_pair(success, seq_num);
}

// delete value in row and column, return sequence number of this operation
std::pair<bool, int> TabletNode::dele(const Request::Request &request,
                                      bool locking) {
  // exclusive lock on log, shared lock tablet
  std::unique_lock log_lck(log_mtx, std::defer_lock);
  std::shared_lock tablet_lck(tablet_mtx, std::defer_lock);

  bool success = false;
  int seq_num = 0;

  if (locking) {
    std::lock(log_lck, tablet_lck);

    // logging
    seq_num = ++seq_num_counter;
    write(curr_lg_fd, request.str.c_str(), sizeof(char) * request.str.size());
    fsync(curr_lg_fd);
    if (++update_counter % cp_freq != 0) {
      log_lck.unlock();
    }
  }

  // delete if row and col exist
  auto row_mtx_itr = rows_mtx.find(request.row);
  if (row_mtx_itr != rows_mtx.end()) {
    // acquire exclusive access to the row
    std::unique_lock row_lck(row_mtx_itr->second);
    auto row_itr = tablet.find(request.row);

    // remove col entry if it exists
    auto val_itr = row_itr->second.find(request.col);
    if (val_itr != row_itr->second.end()) {
      row_itr->second.erase(val_itr);
      success = true;
    }
  }

  // checkpoint if hasn't released log_mtx
  if (locking && log_lck.owns_lock()) {
    checkpoint_tablet(curr_tablet->id, false);
  }

  return std::make_pair(success, seq_num);
}

// return value stored in row and column through parameter val
bool TabletNode::get(const Request::Request &request, std::vector<char> &val) {
  // acquire shared access to the tablet
  std::shared_lock tablet_lck(tablet_mtx);

  // if row does not exist, return false (-1)
  // note: make sure rows_mtx and tablet have the same rows
  auto row_mtx_itr = rows_mtx.find(request.row);
  if (row_mtx_itr == rows_mtx.end()) {
    return false;
  }

  // acquire shared access to the row
  std::shared_lock row_lck(row_mtx_itr->second);
  auto row_itr = tablet.find(request.row);

  // if col does not exist, return false (-1)
  auto val_itr = row_itr->second.find(request.col);
  if (val_itr == row_itr->second.end()) {
    return false;
  }

  // return true (1), and result through argument
  val = val_itr->second;
  return true;
}

// clear memory and states
void TabletNode::clear() {
  seq_num_counter = 0;
  update_counter = 0;
  curr_lg_fd = -1;
  curr_tablet = nullptr;
  tablet.clear();
  rows_mtx.clear();
  tablets.clear();
}
